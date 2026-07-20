/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfnotify/HttpServer.h"
#include "morfnotify/ModuleRegistry.h"
#include "morfnotify/Notification.h"
#include "morfnotify/Version.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDateTime>

#include <utility>

namespace morfnotify {

namespace {
constexpr int kMaxRequestBytes = 65536; // corps d'une notification : petit

QByteArray toJson(const QJsonObject& o) {
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

// Longueur du corps annoncee dans les en-tetes (Content-Length), 0 si absente.
int contentLength(const QByteArray& headerBlock) {
    for (const QByteArray& line : headerBlock.split('\n')) {
        const QByteArray l = line.trimmed();
        if (l.toLower().startsWith("content-length:"))
            return l.mid(l.indexOf(':') + 1).trimmed().toInt();
    }
    return 0;
}
} // namespace

HttpServer::HttpServer(ServiceConfig config, ModuleRegistry* registry, QObject* parent)
    : QObject(parent),
      m_config(std::move(config)),
      m_registry(registry),
      m_server(new QTcpServer(this)) {
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
}

HttpServer::~HttpServer() = default;

bool HttpServer::start() {
    if (m_config.httpPort == 0)
        return false;
    m_uptime.start();
    QHostAddress addr(m_config.bindAddress);
    if (addr.isNull())
        addr = QHostAddress(QHostAddress::AnyIPv4);
    return m_server->listen(addr, m_config.httpPort);
}

void HttpServer::stop()            { m_server->close(); }
bool HttpServer::isListening() const { return m_server->isListening(); }
quint16 HttpServer::port() const   { return m_server->isListening() ? m_server->serverPort() : 0; }

void HttpServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* sock = m_server->nextPendingConnection();
        connect(sock, &QTcpSocket::readyRead, this, [this, sock]() { onSocketReadyRead(sock); });
        connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
    }
}

void HttpServer::onSocketReadyRead(QTcpSocket* sock) {
    QByteArray buf = sock->property("buf").toByteArray();
    buf += sock->readAll();

    const int headerEnd = buf.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        if (buf.size() > kMaxRequestBytes) { sock->abort(); return; }
        sock->setProperty("buf", buf);              // en-tetes incomplets : on attend
        return;
    }

    const QByteArray headerBlock = buf.left(headerEnd);
    const int needed = contentLength(headerBlock);
    const int bodyStart = headerEnd + 4;

    // Attend d'avoir recu tout le corps annonce.
    if (buf.size() - bodyStart < needed) {
        if (buf.size() > kMaxRequestBytes) { sock->abort(); return; }
        sock->setProperty("buf", buf);
        return;
    }

    const int lineEnd = buf.indexOf("\r\n");
    const QList<QByteArray> parts = buf.left(lineEnd).split(' ');
    const QByteArray method = parts.value(0);
    const QByteArray path   = parts.value(1);
    const QByteArray body   = buf.mid(bodyStart, needed);

    sock->setProperty("buf", QByteArray());          // requete consommee
    handleRequest(sock, method, path, body);
}

void HttpServer::handleRequest(QTcpSocket* sock, const QByteArray& method,
                                     const QByteArray& rawPath, const QByteArray& body) {
    const QByteArray path = rawPath.left(rawPath.indexOf('?') < 0 ? rawPath.size()
                                                                  : rawPath.indexOf('?'));
    int        code   = 200;
    QByteArray reason = "OK";
    QByteArray out;

    if (path == "/notify") {
        if (method != "POST") {
            code = 405; reason = "Method Not Allowed";
            out = "{\"error\":\"use POST /notify\"}";
        } else {
            out = handleNotify(body, code, reason);
        }
    } else if (method != "GET") {
        code = 405; reason = "Method Not Allowed";
        out = "{\"error\":\"method not allowed\"}";
    } else if (path == "/healthz") {
        out = "{\"status\":\"ok\"}";
    } else if (path == "/status") {
        out = buildStatusJson();
    } else if (path == "/targets") {
        QJsonObject o;
        o["targets"] = m_registry ? m_registry->targetsJson() : QJsonArray{};
        o["count"]   = m_registry ? m_registry->count() : 0;
        o["ts"]      = static_cast<double>(QDateTime::currentSecsSinceEpoch());
        out = toJson(o);
    } else {
        code = 404; reason = "Not Found";
        out = "{\"error\":\"not found\"}";
    }

    reply(sock, code, reason, out);
}

QByteArray HttpServer::handleNotify(const QByteArray& body, int& code, QByteArray& reason) const {
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        code = 400; reason = "Bad Request";
        return "{\"error\":\"corps JSON invalide\"}";
    }

    Notification n;
    QString err;
    if (!Notification::fromJson(doc.object(), &n, &err)) {
        code = 400; reason = "Bad Request";
        return toJson(QJsonObject{{"error", err}});
    }

    QStringList queued, unknown;
    if (m_registry)
        m_registry->dispatch(n, &queued, &unknown);

    // 202 Accepted : la notification est acceptee et remise en arriere-plan.
    // Les noms de destinations inconnus sont signales mais ne sont pas une erreur.
    code = 202; reason = "Accepted";
    QJsonObject o;
    o["accepted"] = true;
    o["queued"]   = QJsonArray::fromStringList(queued);
    o["unknown"]  = QJsonArray::fromStringList(unknown);
    o["ts"]       = static_cast<double>(n.ts);
    return toJson(o);
}

QByteArray HttpServer::buildStatusJson() const {
    QJsonObject o;
    o["app"]      = m_config.appName;
    o["host"]     = QHostInfo::localHostName();
    o["version"]  = morfnotify::version();
    o["proto"]    = QString::fromLatin1(morfnotify::kProtocol);
    o["state"]    = m_registry ? m_registry->state() : QStringLiteral("ok");
    o["uptime_s"] = static_cast<double>(m_uptime.isValid() ? m_uptime.elapsed() / 1000 : 0);
    o["ts"]       = static_cast<double>(QDateTime::currentSecsSinceEpoch());
    o["metrics"]  = m_registry ? m_registry->metrics() : QJsonObject{};
    return toJson(o);
}

void HttpServer::reply(QTcpSocket* sock, int code, const QByteArray& reason,
                             const QByteArray& body) {
    QByteArray resp;
    resp += "HTTP/1.1 " + QByteArray::number(code) + " " + reason + "\r\n";
    resp += "Content-Type: application/json; charset=utf-8\r\n";
    resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    resp += "Access-Control-Allow-Origin: *\r\n";
    resp += "Connection: close\r\n\r\n";
    resp += body;
    sock->write(resp);
    sock->flush();
    sock->disconnectFromHost();
}

} // namespace morfnotify

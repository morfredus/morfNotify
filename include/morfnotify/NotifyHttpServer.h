/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QByteArray>
#include "morfnotify/NotifyConfig.h"

class QTcpServer;
class QTcpSocket;

namespace morfnotify {

class NotifierRegistry;

// -----------------------------------------------------------------------------
// NotifyHttpServer : serveur HTTP/1.1 local. Contrairement au serveur de
// morfSensor (lecture seule), il accepte un CORPS de requete (POST).
//
//   POST /notify   -> recoit { title, message, level, targets }, valide, et
//                     distribue (reponse 202 Accepted ; 400 si invalide).
//   GET  /targets  -> liste des destinations configurees.
//   GET  /status   -> compatible morfBeacon (app, host, version, state,
//                     uptime_s, metrics, ts).
//   GET  /healthz  -> { "status": "ok" }.
// -----------------------------------------------------------------------------
class NotifyHttpServer : public QObject {
    Q_OBJECT
public:
    NotifyHttpServer(NotifyConfig config, NotifierRegistry* registry,
                     QObject* parent = nullptr);
    ~NotifyHttpServer() override;

    bool start();            // false si le port ne peut etre ouvert
    void stop();
    bool isListening() const;
    quint16 port() const;

private:
    void onNewConnection();
    void onSocketReadyRead(QTcpSocket* sock);
    void handleRequest(QTcpSocket* sock, const QByteArray& method,
                       const QByteArray& path, const QByteArray& body);
    QByteArray handleNotify(const QByteArray& body, int& code, QByteArray& reason) const;
    QByteArray buildStatusJson() const;
    void reply(QTcpSocket* sock, int code, const QByteArray& reason, const QByteArray& body);

    NotifyConfig      m_config;
    NotifierRegistry* m_registry;
    QTcpServer*       m_server;
    QElapsedTimer     m_uptime;
};

} // namespace morfnotify

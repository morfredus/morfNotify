/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfnotify/WebhookNotifier.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>

namespace morfnotify {

namespace {
// Correspondance niveau morfNotify -> priorite ntfy (1 min .. 5 max).
int ntfyPriority(const QString& level) {
    if (level == QLatin1String("error"))   return 5;
    if (level == QLatin1String("warning")) return 4;
    return 3; // info, success, autres
}
// Correspondance niveau -> tag/emoji ntfy.
QByteArray ntfyTags(const QString& level) {
    if (level == QLatin1String("error"))   return "rotating_light";
    if (level == QLatin1String("warning")) return "warning";
    if (level == QLatin1String("success")) return "white_check_mark";
    return "information_source";
}
} // namespace

WebhookNotifier::WebhookNotifier(const QString& name, const QString& url,
                                 const QString& format, QObject* parent)
    : INotifier(name, QStringLiteral("webhook"), parent),
      m_url(url),
      m_format(format.isEmpty() ? QStringLiteral("json") : format.toLower()),
      m_nam(new QNetworkAccessManager(this)) {}

void WebhookNotifier::send(const Notification& n) {
    if (m_format == QLatin1String("ntfy"))
        sendNtfy(n);
    else
        sendJson(n);
}

void WebhookNotifier::sendJson(const Notification& n) {
    QNetworkRequest req{QUrl(m_url)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const QByteArray body = QJsonDocument(n.toJson()).toJson(QJsonDocument::Compact);

    QNetworkReply* reply = m_nam->post(req, body);
    const QString target = name();
    connect(reply, &QNetworkReply::finished, this, [this, reply, target]() {
        if (reply->error() == QNetworkReply::NoError)
            emit delivered(target);
        else
            emit failed(target, reply->errorString());
        reply->deleteLater();
    });
}

void WebhookNotifier::sendNtfy(const Notification& n) {
    // ntfy.sh : corps = message brut, metadonnees en en-tetes.
    QNetworkRequest req{QUrl(m_url)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain; charset=utf-8");
    if (!n.title.isEmpty())
        req.setRawHeader("Title", n.title.toUtf8());
    req.setRawHeader("Priority", QByteArray::number(ntfyPriority(n.level)));
    req.setRawHeader("Tags", ntfyTags(n.level));

    QNetworkReply* reply = m_nam->post(req, n.message.toUtf8());
    const QString target = name();
    connect(reply, &QNetworkReply::finished, this, [this, reply, target]() {
        if (reply->error() == QNetworkReply::NoError)
            emit delivered(target);
        else
            emit failed(target, reply->errorString());
        reply->deleteLater();
    });
}

} // namespace morfnotify

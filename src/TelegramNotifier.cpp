/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfnotify/TelegramNotifier.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace morfnotify {

namespace {
QString levelLabel(const QString& level) {
    if (level == QLatin1String("error")) return QStringLiteral("ERREUR");
    if (level == QLatin1String("warning")) return QStringLiteral("ALERTE");
    if (level == QLatin1String("success")) return QStringLiteral("OK");
    return QStringLiteral("INFO");
}
} // namespace

TelegramNotifier::TelegramNotifier(const QString& name, const QString& botToken,
                                   const QString& chatId, QObject* parent)
    : INotifier(name, QStringLiteral("telegram"), parent),
      m_botToken(botToken),
      m_chatId(chatId),
      m_nam(new QNetworkAccessManager(this)) {}

void TelegramNotifier::send(const Notification& notification) {
    const QUrl url(QStringLiteral("https://api.telegram.org/bot%1/sendMessage").arg(m_botToken));
    QNetworkRequest req{url};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["chat_id"] = m_chatId;
    body["text"] = formatMessage(notification);
    body["disable_web_page_preview"] = true;

    QNetworkReply* reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    const QString target = name();
    connect(reply, &QNetworkReply::finished, this, [this, reply, target]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit delivered(target);
        } else {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QString body = QString::fromUtf8(reply->readAll());
            QString error = reply->errorString();
            if (status > 0)
                error += QStringLiteral(" (HTTP %1)").arg(status);
            if (!body.trimmed().isEmpty())
                error += QStringLiteral(" : %1").arg(body.trimmed());
            emit failed(target, error);
        }
        reply->deleteLater();
    });
}

QString TelegramNotifier::formatMessage(const Notification& notification) const {
    QString text = QStringLiteral("[%1]").arg(levelLabel(notification.level));
    if (!notification.title.isEmpty())
        text += QStringLiteral(" %1").arg(notification.title);
    text += QStringLiteral("\n\n%1").arg(notification.message);
    return text;
}

} // namespace morfnotify

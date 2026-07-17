/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfnotify/NotifierFactory.h"
#include "morfnotify/EmailNotifier.h"
#include "morfnotify/INotifier.h"
#include "morfnotify/LogNotifier.h"
#include "morfnotify/TelegramNotifier.h"
#include "morfnotify/WebhookNotifier.h"

namespace morfnotify {
namespace NotifierFactory {

// -----------------------------------------------------------------------------
// POUR AJOUTER UNE DESTINATION :
//   1. ecrire la classe (heriter d'INotifier) ;
//   2. ajouter une branche dans create() qui lit ses parametres ;
//   3. ajouter son nom dans knownTypes().
// Aucune autre partie du code (registre, serveur HTTP, service) ne change.
// -----------------------------------------------------------------------------

INotifier* create(const TargetDef& def, QString* error, QObject* parent) {
    const QString type = def.type.toLower();

    if (type == QLatin1String("log")) {
        const QString file = def.params.value("file").toString();
        return new LogNotifier(def.name, file, parent);
    }

    if (type == QLatin1String("webhook")) {
        const QString url = def.params.value("url").toString();
        if (url.isEmpty()) {
            if (error) *error = QStringLiteral("destination '%1' : parametre 'url' manquant").arg(def.name);
            return nullptr;
        }
        const QString format = def.params.value("format").toString();
        return new WebhookNotifier(def.name, url, format, parent);
    }

    if (type == QLatin1String("email")) {
        EmailNotifier::Config config;
        config.host = def.params.value("smtp_host").toString();
        config.port = static_cast<quint16>(def.params.value("smtp_port").toInt(587));
        config.security = def.params.value("security").toString(QStringLiteral("starttls")).toLower();
        config.username = def.params.value("username").toString();
        config.password = def.params.value("password").toString();
        config.from = def.params.value("from").toString();
        config.subjectPrefix = def.params.value("subject_prefix").toString();
        config.timeoutMs = def.params.value("timeout_ms").toInt(15000);

        const QJsonValue toValue = def.params.value("to");
        if (toValue.isArray()) {
            const QJsonArray recipients = toValue.toArray();
            for (const QJsonValue& recipient : recipients) {
                const QString address = recipient.toString().trimmed();
                if (!address.isEmpty())
                    config.to << address;
            }
        } else {
            const QString address = toValue.toString().trimmed();
            if (!address.isEmpty())
                config.to << address;
        }

        if (config.host.isEmpty() || config.from.isEmpty() || config.to.isEmpty()) {
            if (error) *error = QStringLiteral("destination '%1' : parametres email smtp_host/from/to requis").arg(def.name);
            return nullptr;
        }
        if (config.security != QLatin1String("starttls") &&
            config.security != QLatin1String("ssl") &&
            config.security != QLatin1String("none")) {
            if (error) *error = QStringLiteral("destination '%1' : security doit valoir starttls, ssl ou none").arg(def.name);
            return nullptr;
        }

        return new EmailNotifier(def.name, config, parent);
    }

    if (type == QLatin1String("telegram")) {
        const QString botToken = def.params.value("bot_token").toString();
        const QString chatId = def.params.value("chat_id").toString();
        if (botToken.isEmpty() || chatId.isEmpty()) {
            if (error) *error = QStringLiteral("destination '%1' : parametres telegram bot_token/chat_id requis").arg(def.name);
            return nullptr;
        }
        return new TelegramNotifier(def.name, botToken, chatId, parent);
    }

    if (error)
        *error = QStringLiteral("type de destination inconnu : '%1'").arg(def.type);
    return nullptr;
}

QStringList knownTypes() {
    return {
        QStringLiteral("log"),
        QStringLiteral("webhook"),
        QStringLiteral("email"),
        QStringLiteral("telegram"),
    };
}

} // namespace NotifierFactory
} // namespace morfnotify

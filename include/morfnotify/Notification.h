/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDateTime>

namespace morfnotify {

// -----------------------------------------------------------------------------
// Notification : le message a diffuser. C'est le SEUL objet metier de morfNotify,
// et il est volontairement pauvre : morfNotify ne l'interprete pas, il le relaie.
//
//   title    : titre court (ex. "morfSensor")
//   message  : corps du message
//   level    : "info" | "success" | "warning" | "error" (libre, defaut "info")
//   targets  : noms des destinations demandees (ex. ["pixel", "dashboard"])
//   ts       : horodatage de reception (rempli par le service)
//
// Le projet appelant decide quand/quoi/a qui envoyer. morfNotify se contente de
// valider la forme puis de distribuer.
// -----------------------------------------------------------------------------
struct Notification {
    QString     title;
    QString     message;
    QString     level = QStringLiteral("info");
    QStringList targets;
    qint64      ts = 0;

    // Niveaux reconnus (pour normalisation / couleur cote destination). Toute
    // autre valeur est acceptee telle quelle.
    static QStringList knownLevels() {
        return { QStringLiteral("info"), QStringLiteral("success"),
                 QStringLiteral("warning"), QStringLiteral("error") };
    }

    // Construit une Notification depuis un objet JSON (corps de POST /notify).
    // Renvoie false + renseigne `error` si la forme est invalide.
    static bool fromJson(const QJsonObject& o, Notification* out, QString* error) {
        Notification n;
        n.title   = o.value(QStringLiteral("title")).toString();
        n.message = o.value(QStringLiteral("message")).toString();

        if (n.message.trimmed().isEmpty()) {
            if (error) *error = QStringLiteral("champ 'message' requis et non vide");
            return false;
        }

        const QString lvl = o.value(QStringLiteral("level")).toString();
        if (!lvl.isEmpty())
            n.level = lvl;

        // 'targets' : tableau de chaines, ou une seule chaine par commodite.
        const QJsonValue t = o.value(QStringLiteral("targets"));
        if (t.isArray()) {
            for (const QJsonValue& v : t.toArray()) {
                const QString name = v.toString().trimmed();
                if (!name.isEmpty())
                    n.targets << name;
            }
        } else if (t.isString()) {
            const QString name = t.toString().trimmed();
            if (!name.isEmpty())
                n.targets << name;
        }

        if (n.targets.isEmpty()) {
            if (error) *error = QStringLiteral("champ 'targets' requis (au moins une destination)");
            return false;
        }

        n.ts = QDateTime::currentSecsSinceEpoch();
        *out = n;
        return true;
    }

    // Serialisation (transmise telle quelle aux destinations de type webhook).
    QJsonObject toJson() const {
        QJsonObject o;
        o["title"]   = title;
        o["message"] = message;
        o["level"]   = level;
        o["targets"] = QJsonArray::fromStringList(targets);
        o["ts"]      = static_cast<double>(ts);
        return o;
    }
};

} // namespace morfnotify

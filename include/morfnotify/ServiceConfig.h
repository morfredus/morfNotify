/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QtGlobal>

namespace morfnotify {

// -----------------------------------------------------------------------------
// ModuleDef : declaration d'UNE destination a activer (lue depuis la config).
//
//   name   : identifiant utilise par les producteurs dans "targets" ("pixel")
//   type   : identifiant de fabrique ("log", "webhook", ...) -> ModuleFactory
//   params : objet JSON libre, propre au type (url, fichier, format...)
// -----------------------------------------------------------------------------
struct ModuleDef {
    QString     name;
    QString     type;
    QJsonObject params;
};

// -----------------------------------------------------------------------------
// ServiceConfig : configuration complete du service. Charge depuis un fichier
// JSON (voir config/morfnotify.example.json).
// -----------------------------------------------------------------------------
struct ServiceConfig {
    // Identite annoncee (heartbeat morfBeacon + /status).
    QString appName    = QStringLiteral("morfNotify");
    QString instanceId;                              // defaut = appName@hostname

    // Serveur HTTP local (API de reception).
    quint16 httpPort    = 8789;                      // 0 => pas de serveur HTTP
    QString bindAddress = QStringLiteral("0.0.0.0"); // interfaces ecoutees

    // Annonce de presence sur le LAN via morfBeacon.
    bool    beaconEnabled    = true;
    quint16 beaconUdpPort    = 45454;                // port du parc (cf. dashboard)
    int     beaconIntervalMs = 15000;

    // Destinations a activer.
    QVector<ModuleDef> targets;

    static ServiceConfig fromJson(const QJsonObject& root) {
        ServiceConfig c;
        if (root.contains("app_name"))     c.appName     = root.value("app_name").toString(c.appName);
        if (root.contains("instance_id"))  c.instanceId  = root.value("instance_id").toString();
        if (root.contains("http_port"))    c.httpPort    = static_cast<quint16>(root.value("http_port").toInt(c.httpPort));
        if (root.contains("bind_address")) c.bindAddress = root.value("bind_address").toString(c.bindAddress);

        const QJsonObject beacon = root.value("beacon").toObject();
        if (beacon.contains("enabled"))     c.beaconEnabled    = beacon.value("enabled").toBool(c.beaconEnabled);
        if (beacon.contains("udp_port"))    c.beaconUdpPort    = static_cast<quint16>(beacon.value("udp_port").toInt(c.beaconUdpPort));
        if (beacon.contains("interval_ms")) c.beaconIntervalMs = beacon.value("interval_ms").toInt(c.beaconIntervalMs);

        const QJsonArray targets = root.value("targets").toArray();
        for (const QJsonValue& v : targets) {
            const QJsonObject o = v.toObject();
            const QString type = o.value("type").toString();
            const QString name = o.value("name").toString();
            if (type.isEmpty() || name.isEmpty())
                continue;                            // entree invalide : ignoree
            ModuleDef d;
            d.name   = name;
            d.type   = type;
            d.params = o;
            c.targets.push_back(d);
        }
        return c;
    }
};

} // namespace morfnotify

/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfnotify/Service.h"
#include "morfnotify/ModuleRegistry.h"
#include "morfnotify/HttpServer.h"
#include "morfnotify/ModuleFactory.h"
#include "morfnotify/INotifier.h"
#include "morfnotify/Version.h"

#include "morfbeacon/Heartbeat.h"
#include "morfbeacon/PresenceConfig.h"

#include <QDebug>
#include <utility>

namespace morfnotify {

Service::Service(ServiceConfig config, QObject* parent)
    : QObject(parent),
      m_config(std::move(config)),
      m_registry(new ModuleRegistry(this)),
      m_http(nullptr) {

    // Construit les destinations declarees. Une erreur sur l'une (type inconnu,
    // parametre manquant, nom en double) n'empeche pas les autres : on la note.
    for (const ModuleDef& def : m_config.targets) {
        QString error;
        INotifier* n = ModuleFactory::create(def, &error);
        if (!n) {
            m_warnings << error;
            continue;
        }
        if (!m_registry->add(n)) {
            m_warnings << QStringLiteral("destination '%1' ignoree (nom en double)").arg(def.name);
            delete n;
        }
    }

    // Journalise les livraisons/echecs (utile en service via journald).
    connect(m_registry, &ModuleRegistry::failed, this,
            [](const QString& t, const QString& e) {
                qWarning().noquote() << "morfNotify: echec livraison vers" << t << ":" << e;
            });

    m_http = new HttpServer(m_config, m_registry, this);
}

Service::~Service() = default;

bool Service::start() {
    const bool httpOk = (m_config.httpPort == 0) ? true : m_http->start();

    if (m_config.beaconEnabled) {
        morfbeacon::PresenceConfig pc;
        pc.appName             = m_config.appName;
        pc.version             = morfnotify::version();
        pc.instanceId          = m_config.instanceId;
        pc.udpPort             = m_config.beaconUdpPort;
        pc.broadcastIntervalMs = m_config.beaconIntervalMs;
        pc.statusPort          = m_http ? m_http->port() : 0;
        pc.statusBindAddress   = m_config.bindAddress;

        m_heartbeat = new morfbeacon::Heartbeat(pc, m_registry, this);
        m_heartbeat->start();
    }

    return httpOk;
}

void Service::stop() {
    if (m_heartbeat)
        m_heartbeat->stop();
    if (m_http)
        m_http->stop();
}

int Service::targetCount() const   { return m_registry->count(); }
quint16 Service::httpPort() const  { return m_http ? m_http->port() : 0; }
QStringList Service::warnings() const { return m_warnings; }
ModuleRegistry* Service::registry() const { return m_registry; }

} // namespace morfnotify

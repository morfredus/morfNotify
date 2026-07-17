/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfnotify/NotifyService.h"
#include "morfnotify/NotifierRegistry.h"
#include "morfnotify/NotifyHttpServer.h"
#include "morfnotify/NotifierFactory.h"
#include "morfnotify/INotifier.h"
#include "morfnotify/Version.h"

#include "morfbeacon/Heartbeat.h"
#include "morfbeacon/PresenceConfig.h"

#include <QDebug>
#include <utility>

namespace morfnotify {

NotifyService::NotifyService(NotifyConfig config, QObject* parent)
    : QObject(parent),
      m_config(std::move(config)),
      m_registry(new NotifierRegistry(this)),
      m_http(nullptr) {

    // Construit les destinations declarees. Une erreur sur l'une (type inconnu,
    // parametre manquant, nom en double) n'empeche pas les autres : on la note.
    for (const TargetDef& def : m_config.targets) {
        QString error;
        INotifier* n = NotifierFactory::create(def, &error);
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
    connect(m_registry, &NotifierRegistry::failed, this,
            [](const QString& t, const QString& e) {
                qWarning().noquote() << "morfNotify: echec livraison vers" << t << ":" << e;
            });

    m_http = new NotifyHttpServer(m_config, m_registry, this);
}

NotifyService::~NotifyService() = default;

bool NotifyService::start() {
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

void NotifyService::stop() {
    if (m_heartbeat)
        m_heartbeat->stop();
    if (m_http)
        m_http->stop();
}

int NotifyService::targetCount() const   { return m_registry->count(); }
quint16 NotifyService::httpPort() const  { return m_http ? m_http->port() : 0; }
QStringList NotifyService::warnings() const { return m_warnings; }
NotifierRegistry* NotifyService::registry() const { return m_registry; }

} // namespace morfnotify

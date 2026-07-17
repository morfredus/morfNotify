/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QObject>
#include <QStringList>
#include "morfnotify/NotifyConfig.h"

namespace morfbeacon { class Heartbeat; }

namespace morfnotify {

class NotifierRegistry;
class NotifyHttpServer;

// -----------------------------------------------------------------------------
// NotifyService : facade qui cable tout le service a partir d'une NotifyConfig.
//
//   config JSON
//     -> destinations (NotifierFactory) -> NotifierRegistry
//     -> NotifyHttpServer (POST /notify, /targets, /status)
//     -> morfbeacon::Heartbeat (annonce de presence sur le LAN)
//
// Seul objet manipule par le demon (service/main.cpp).
// -----------------------------------------------------------------------------
class NotifyService : public QObject {
    Q_OBJECT
public:
    explicit NotifyService(NotifyConfig config, QObject* parent = nullptr);
    ~NotifyService() override;

    bool start();
    void stop();

    int         targetCount() const;
    quint16     httpPort() const;
    QStringList warnings() const;                    // erreurs de construction des cibles

    NotifierRegistry* registry() const;

private:
    NotifyConfig            m_config;
    NotifierRegistry*       m_registry;
    NotifyHttpServer*       m_http;
    morfbeacon::Heartbeat*  m_heartbeat = nullptr;
    QStringList             m_warnings;
};

} // namespace morfnotify

/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include "morfnotify/INotifier.h"

class QNetworkAccessManager;

namespace morfnotify {

// -----------------------------------------------------------------------------
// WebhookNotifier : destination qui POSTe la notification vers une URL HTTP.
//
// Generique : couvre un dashboard web, une passerelle push, ntfy.sh, etc. La
// livraison est asynchrone (QNetworkAccessManager) et non bloquante.
//
// Parametres (ModuleDef::params) :
//   "url"    : URL cible (REQUIS).
//   "format" : "json" (defaut) -> POST le JSON complet de la notification ;
//              "ntfy" -> corps = message, en-tetes Title/Priority/Tags (ntfy.sh,
//              pratique pour une notification push sur un Pixel).
// -----------------------------------------------------------------------------
class WebhookNotifier : public INotifier {
    Q_OBJECT
public:
    WebhookNotifier(const QString& name, const QString& url,
                    const QString& format = QStringLiteral("json"),
                    QObject* parent = nullptr);

    void send(const Notification& notification) override;

private:
    void sendJson(const Notification& n);
    void sendNtfy(const Notification& n);

    QString                m_url;
    QString                m_format;
    QNetworkAccessManager* m_nam;
};

} // namespace morfnotify

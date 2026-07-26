/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include "morfbeacon/PresenceConfig.h"

namespace morfnotify {

// -----------------------------------------------------------------------------
// fillAnnouncedDetail : renseigne le DETAIL annonce du service dans un
// PresenceConfig, pour que morfbeacon::describeService le serialise dans /status.
//
// morfNotify est un service SANS interface web : il n'annonce donc pas de
// `web_ui` (aucun `webUiPath`), seulement la liste de son API. describeService
// n'emettra que le bloc `api` -- pas de cle `web_ui` fantome.
//
// Point UNIQUE : la meme fonction sert de source a tout observateur du parc.
// Le jour ou ce service exposerait une interface, elle se declarerait ICI, une
// seule fois, a cote de l'API -- comme le font deja morfAnalytics et morfMonitor.
//
// Le heartbeat morfBeacon, lui, reste maigre : il annonce des CAPACITES, pas
// l'API. C'est pourquoi Service.cpp n'appelle pas cette fonction : le detail
// (API et, le cas echeant, interface) ne vit que dans /status, jamais dans le
// datagramme de presence.
//
// En-tete (inline) : aucun fichier source ni entree CMake supplementaires.
inline void fillAnnouncedDetail(morfbeacon::PresenceConfig& pc) {
    // API metier. Les routes de cadre -- /status, /healthz -- ne sont pas
    // listees : un observateur les connait deja par le protocole. Chemins a la
    // racine, donc pas de prefixe commun (apiBasePath).
    pc.api = {
        {QStringLiteral("POST"), QStringLiteral("/notify"),
         QStringLiteral("remettre une notification aux destinations configurees")},
        {QStringLiteral("GET"),  QStringLiteral("/targets"),
         QStringLiteral("destinations configurees et leur etat")},
    };
}

} // namespace morfnotify

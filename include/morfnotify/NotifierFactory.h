/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>
#include <QStringList>
#include "morfnotify/NotifyConfig.h"

class QObject;

namespace morfnotify {

class INotifier;

// -----------------------------------------------------------------------------
// NotifierFactory : fabrique un INotifier a partir d'une declaration (TargetDef).
//
// Point d'extension COMPILE-TIME : pour ajouter un canal (e-mail SMTP, MQTT,
// ecran dedie, ...), on ecrit sa classe INotifier puis on ajoute une branche
// ici. L'API HTTP et les producteurs ne changent jamais.
// -----------------------------------------------------------------------------
namespace NotifierFactory {

// Cree la destination correspondant a `def`. Renvoie nullptr si le type est
// inconnu ou mal configure ; `error` (optionnel) est alors renseigne.
INotifier* create(const TargetDef& def, QString* error = nullptr, QObject* parent = nullptr);

// Types de destinations connus (ex. {"log", "webhook"}).
QStringList knownTypes();

} // namespace NotifierFactory

} // namespace morfnotify

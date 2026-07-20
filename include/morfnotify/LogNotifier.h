/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include "morfnotify/INotifier.h"

namespace morfnotify {

// -----------------------------------------------------------------------------
// LogNotifier : destination qui JOURNALISE la notification.
//
// Ecrit toujours dans le journal (qInfo -> journald sous systemd) et, si un
// fichier est configure, y ajoute une ligne horodatee. Sans dependance ni
// reseau : destination par defaut ideale, et exemple minimal du pattern.
//
// Parametres (ModuleDef::params) :
//   "file" : chemin d'un fichier de log (optionnel ; journal seul si absent).
// -----------------------------------------------------------------------------
class LogNotifier : public INotifier {
    Q_OBJECT
public:
    LogNotifier(const QString& name, const QString& file = QString(),
                QObject* parent = nullptr);

    void send(const Notification& notification) override;

private:
    QString m_file;
};

} // namespace morfnotify

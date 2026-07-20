/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QObject>
#include <QHash>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include "morfbeacon/IMetricsProvider.h"

namespace morfnotify {

class INotifier;
struct Notification;

// -----------------------------------------------------------------------------
// ModuleRegistry : collection des destinations et point de DISTRIBUTION.
//
// - detient les INotifier (en devient le parent Qt), indexes par nom ;
// - dispatch() remet une notification vers les destinations demandees, en
//   ignorant proprement les noms inconnus (rapportes a l'appelant) ;
// - tient un compteur simple (recu / distribue / echecs) expose via /status
//   (implemente morfbeacon::IMetricsProvider).
//
// La livraison est asynchrone : dispatch() ne bloque pas, il declenche l'envoi
// de chaque destination et retourne aussitot la liste des cibles retenues.
// -----------------------------------------------------------------------------
class ModuleRegistry : public QObject, public morfbeacon::IMetricsProvider {
    Q_OBJECT
public:
    explicit ModuleRegistry(QObject* parent = nullptr);
    ~ModuleRegistry() override;

    // Ajoute une destination (le registre en prend possession). Renvoie false si
    // le nom est deja pris (les noms doivent etre uniques).
    bool add(INotifier* notifier);

    int  count() const;
    QStringList targetNames() const;

    // Distribue `n` vers n.targets. Remplit `queued` (destinations retenues et
    // declenchees) et `unknown` (noms demandes mais non configures).
    void dispatch(const Notification& n, QStringList* queued, QStringList* unknown);

    QJsonArray targetsJson() const;                  // { name, type } pour /targets

    // --- morfbeacon::IMetricsProvider ------------------------------------
    QJsonObject metrics() const override;            // compteurs pour /status
    QString     state() const override;

signals:
    void delivered(const QString& target);
    void failed(const QString& target, const QString& error);

private:
    QHash<QString, INotifier*> m_targets;            // name -> notifier
    quint64 m_received  = 0;
    quint64 m_dispatched = 0;                        // envois declenches (cible x message)
    quint64 m_failures  = 0;
};

} // namespace morfnotify

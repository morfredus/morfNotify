/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QObject>
#include <QString>
#include "morfnotify/Notification.h"

namespace morfnotify {

// -----------------------------------------------------------------------------
// INotifier : LE point d'extension de morfNotify.
//
// Chaque DESTINATION (push Pixel, dashboard, e-mail, ecran cuisine, ...) est une
// classe qui herite d'INotifier et sait remettre un message. La livraison est
// ASYNCHRONE et « fire-and-forget » : le service accepte la notification puis
// la remet en arriere-plan, sans faire attendre l'appelant. Le resultat est
// signale (pour journalisation), jamais renvoye en bloquant.
//
// Ajouter une destination = ecrire une sous-classe ici + l'enregistrer dans
// NotifierFactory. L'API HTTP et les projets producteurs ne changent pas :
// c'est tout l'interet du point de diffusion unique.
// -----------------------------------------------------------------------------
class INotifier : public QObject {
    Q_OBJECT
public:
    // name : identifiant unique de la destination ("pixel", "dashboard"...),
    //        c'est ce que les producteurs mettent dans "targets".
    // type : identifiant de fabrique ("log", "webhook", ...).
    INotifier(QString name, QString type, QObject* parent = nullptr)
        : QObject(parent), m_name(std::move(name)), m_type(std::move(type)) {}
    ~INotifier() override = default;

    QString name() const { return m_name; }
    QString type() const { return m_type; }

    // Remet le message vers cette destination. Ne doit PAS bloquer : lancer
    // l'envoi et emettre delivered()/failed() a la fin.
    virtual void send(const Notification& notification) = 0;

signals:
    void delivered(const QString& target);
    void failed(const QString& target, const QString& error);

private:
    QString m_name;
    QString m_type;
};

} // namespace morfnotify

/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfnotify/NotifierRegistry.h"
#include "morfnotify/INotifier.h"
#include "morfnotify/Notification.h"

namespace morfnotify {

NotifierRegistry::NotifierRegistry(QObject* parent) : QObject(parent) {}
NotifierRegistry::~NotifierRegistry() = default;

bool NotifierRegistry::add(INotifier* notifier) {
    if (!notifier)
        return false;
    if (m_targets.contains(notifier->name()))
        return false;                                // nom deja pris

    notifier->setParent(this);
    m_targets.insert(notifier->name(), notifier);

    connect(notifier, &INotifier::delivered, this, [this](const QString& t) {
        emit delivered(t);
    });
    connect(notifier, &INotifier::failed, this, [this](const QString& t, const QString& e) {
        ++m_failures;
        emit failed(t, e);
    });
    return true;
}

int NotifierRegistry::count() const {
    return m_targets.size();
}

QStringList NotifierRegistry::targetNames() const {
    return m_targets.keys();
}

void NotifierRegistry::dispatch(const Notification& n, QStringList* queued, QStringList* unknown) {
    ++m_received;
    for (const QString& name : n.targets) {
        auto it = m_targets.constFind(name);
        if (it == m_targets.constEnd()) {
            if (unknown) *unknown << name;           // destination non configuree
            continue;
        }
        it.value()->send(n);                         // asynchrone, ne bloque pas
        ++m_dispatched;
        if (queued) *queued << name;
    }
}

QJsonArray NotifierRegistry::targetsJson() const {
    QJsonArray arr;
    for (const INotifier* t : m_targets) {
        QJsonObject o;
        o["name"] = t->name();
        o["type"] = t->type();
        arr.append(o);
    }
    return arr;
}

QJsonObject NotifierRegistry::metrics() const {
    QJsonObject m;
    m["targets"]            = m_targets.size();
    m["received_total"]     = static_cast<double>(m_received);
    m["dispatched_total"]   = static_cast<double>(m_dispatched);
    m["failures_total"]     = static_cast<double>(m_failures);
    return m;
}

QString NotifierRegistry::state() const {
    if (m_targets.isEmpty())
        return QStringLiteral("warning");            // aucun canal de sortie configure
    return QStringLiteral("ok");
}

} // namespace morfnotify

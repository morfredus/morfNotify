/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfnotify/LogNotifier.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

namespace morfnotify {

LogNotifier::LogNotifier(const QString& name, const QString& file, QObject* parent)
    : INotifier(name, QStringLiteral("log"), parent), m_file(file) {}

void LogNotifier::send(const Notification& n) {
    const QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    const QString line = QStringLiteral("[%1] %2/%3 - %4 : %5")
                             .arg(stamp, name(), n.level, n.title, n.message);

    // Journal (repris par journald quand on tourne en service systemd).
    qInfo().noquote() << QStringLiteral("morfNotify") << line;

    if (!m_file.isEmpty()) {
        QFile f(m_file);
        if (f.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << line << '\n';
        } else {
            emit failed(name(), QStringLiteral("ecriture impossible dans ") + m_file);
            return;
        }
    }

    emit delivered(name());
}

} // namespace morfnotify

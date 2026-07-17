/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include "morfnotify/INotifier.h"

class QNetworkAccessManager;

namespace morfnotify {

class TelegramNotifier : public INotifier {
    Q_OBJECT
public:
    TelegramNotifier(const QString& name, const QString& botToken,
                     const QString& chatId, QObject* parent = nullptr);
    void send(const Notification& notification) override;

private:
    QString formatMessage(const Notification& notification) const;

    QString m_botToken;
    QString m_chatId;
    QNetworkAccessManager* m_nam;
};

} // namespace morfnotify

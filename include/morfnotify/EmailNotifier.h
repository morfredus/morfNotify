/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include "morfnotify/INotifier.h"

#include <QQueue>
#include <QSslSocket>
#include <QStringList>
#include <QTimer>

namespace morfnotify {

class EmailNotifier : public INotifier {
    Q_OBJECT
public:
    struct Config {
        QString host;
        quint16 port = 587;
        QString security = QStringLiteral("starttls"); // starttls | ssl | none
        QString username;
        QString password;
        QString from;
        QStringList to;
        QString subjectPrefix;
        int timeoutMs = 15000;
    };

    EmailNotifier(const QString& name, Config config, QObject* parent = nullptr);
    void send(const Notification& notification) override;

private:
    enum class Step {
        Greeting,
        EhloBeforeTls,
        StartTls,
        EhloAfterTls,
        AuthUser,
        AuthPassword,
        MailFrom,
        RcptTo,
        DataCommand,
        DataBody,
        Quit
    };

    struct PendingMail {
        Notification notification;
        QByteArray data;
    };

    void startNext();
    void connectSocket();
    void failCurrent(const QString& error);
    void finishCurrent();
    void writeLine(const QByteArray& line);
    void handleReadyRead();
    void handleResponse(int code, const QByteArray& text);
    void advanceAfterEhlo();
    void resetSocket();
    QByteArray buildMessage(const Notification& notification) const;
    QString subjectFor(const Notification& notification) const;

    Config m_config;
    QQueue<PendingMail> m_queue;
    PendingMail m_current;
    QSslSocket* m_socket = nullptr;
    QTimer m_timeout;
    QByteArray m_buffer;
    Step m_step = Step::Greeting;
    int m_rcptIndex = 0;
    bool m_busy = false;
};

} // namespace morfnotify

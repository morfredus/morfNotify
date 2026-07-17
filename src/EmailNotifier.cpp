/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfnotify/EmailNotifier.h"

#include <QDateTime>
#include <QHostInfo>
#include <QRegularExpression>
#include <QSslSocket>
#include <QTextStream>

namespace morfnotify {

namespace {
QByteArray b64(const QString& value) {
    return value.toUtf8().toBase64();
}

QByteArray dotStuffed(QByteArray body) {
    body.replace("\r\n.", "\r\n..");
    if (body.startsWith('.'))
        body.prepend('.');
    return body;
}

QString cleanHeader(QString value) {
    value.replace('\r', ' ');
    value.replace('\n', ' ');
    return value.trimmed();
}

QString mailboxAddress(const QString& value) {
    const QRegularExpression re(QStringLiteral("<([^>]+)>"));
    const QRegularExpressionMatch match = re.match(value);
    if (match.hasMatch())
        return match.captured(1).trimmed();
    return value.trimmed();
}

QString levelLabel(const QString& level) {
    if (level == QLatin1String("error")) return QStringLiteral("ERREUR");
    if (level == QLatin1String("warning")) return QStringLiteral("ALERTE");
    if (level == QLatin1String("success")) return QStringLiteral("OK");
    return QStringLiteral("INFO");
}
} // namespace

EmailNotifier::EmailNotifier(const QString& name, Config config, QObject* parent)
    : INotifier(name, QStringLiteral("email"), parent),
      m_config(std::move(config)) {
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this]() {
        failCurrent(QStringLiteral("timeout SMTP"));
    });
}

void EmailNotifier::send(const Notification& notification) {
    PendingMail mail;
    mail.notification = notification;
    mail.data = buildMessage(notification);
    m_queue.enqueue(mail);
    if (!m_busy)
        startNext();
}

void EmailNotifier::startNext() {
    if (m_queue.isEmpty()) {
        m_busy = false;
        return;
    }

    m_busy = true;
    m_current = m_queue.dequeue();
    m_step = Step::Greeting;
    m_rcptIndex = 0;
    m_buffer.clear();
    connectSocket();
}

void EmailNotifier::connectSocket() {
    resetSocket();
    m_socket = new QSslSocket(this);
    connect(m_socket, &QSslSocket::readyRead, this, &EmailNotifier::handleReadyRead);
    connect(m_socket, &QSslSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        failCurrent(m_socket ? m_socket->errorString() : QStringLiteral("erreur socket SMTP"));
    });
    connect(m_socket, &QSslSocket::sslErrors, this, [this](const QList<QSslError>& errors) {
        QStringList messages;
        for (const QSslError& e : errors)
            messages << e.errorString();
        failCurrent(messages.join(QStringLiteral("; ")));
    });

    m_timeout.start(m_config.timeoutMs);
    if (m_config.security == QLatin1String("ssl"))
        m_socket->connectToHostEncrypted(m_config.host, m_config.port);
    else
        m_socket->connectToHost(m_config.host, m_config.port);
}

void EmailNotifier::resetSocket() {
    if (!m_socket)
        return;
    m_socket->disconnect(this);
    m_socket->deleteLater();
    m_socket = nullptr;
}

void EmailNotifier::failCurrent(const QString& error) {
    m_timeout.stop();
    resetSocket();
    emit failed(name(), error);
    startNext();
}

void EmailNotifier::finishCurrent() {
    m_timeout.stop();
    resetSocket();
    emit delivered(name());
    startNext();
}

void EmailNotifier::writeLine(const QByteArray& line) {
    if (!m_socket)
        return;
    m_socket->write(line);
    m_socket->write("\r\n");
}

void EmailNotifier::handleReadyRead() {
    if (!m_socket)
        return;

    m_buffer += m_socket->readAll();
    while (true) {
        const int pos = m_buffer.indexOf("\r\n");
        if (pos < 0)
            return;

        const QByteArray line = m_buffer.left(pos);
        m_buffer.remove(0, pos + 2);
        if (line.size() < 4)
            continue;

        bool ok = false;
        const int code = line.left(3).toInt(&ok);
        if (!ok)
            continue;

        if (line.size() > 3 && line.at(3) == '-')
            continue; // reponse multi-ligne : attendre la derniere ligne "250 ..."

        handleResponse(code, line.mid(4));
    }
}

void EmailNotifier::handleResponse(int code, const QByteArray& text) {
    Q_UNUSED(text);

    const QByteArray localName = QHostInfo::localHostName().toUtf8();
    switch (m_step) {
    case Step::Greeting:
        if (code != 220) return failCurrent(QStringLiteral("SMTP: accueil inattendu %1").arg(code));
        m_step = Step::EhloBeforeTls;
        writeLine("EHLO " + localName);
        return;

    case Step::EhloBeforeTls:
        if (code != 250) return failCurrent(QStringLiteral("SMTP: EHLO refuse %1").arg(code));
        if (m_config.security == QLatin1String("starttls")) {
            m_step = Step::StartTls;
            writeLine("STARTTLS");
        } else {
            advanceAfterEhlo();
        }
        return;

    case Step::StartTls:
        if (code != 220) return failCurrent(QStringLiteral("SMTP: STARTTLS refuse %1").arg(code));
        connect(m_socket, &QSslSocket::encrypted, this, [this, localName]() {
            m_step = Step::EhloAfterTls;
            writeLine("EHLO " + localName);
        });
        m_socket->startClientEncryption();
        return;

    case Step::EhloAfterTls:
        if (code != 250) return failCurrent(QStringLiteral("SMTP: EHLO TLS refuse %1").arg(code));
        advanceAfterEhlo();
        return;

    case Step::AuthUser:
        if (code != 334) return failCurrent(QStringLiteral("SMTP: AUTH utilisateur refuse %1").arg(code));
        m_step = Step::AuthPassword;
        writeLine(b64(m_config.username));
        return;

    case Step::AuthPassword:
        if (code != 334) return failCurrent(QStringLiteral("SMTP: AUTH mot de passe refuse %1").arg(code));
        m_step = Step::MailFrom;
        writeLine(b64(m_config.password));
        return;

    case Step::MailFrom:
        if (code != 235 && code != 250) return failCurrent(QStringLiteral("SMTP: authentification refusee %1").arg(code));
        m_step = Step::RcptTo;
        writeLine("MAIL FROM:<" + mailboxAddress(m_config.from).toUtf8() + ">");
        return;

    case Step::RcptTo:
        if (code != 250) return failCurrent(QStringLiteral("SMTP: expediteur refuse %1").arg(code));
        m_step = Step::DataCommand;
        writeLine("RCPT TO:<" + mailboxAddress(m_config.to.value(m_rcptIndex)).toUtf8() + ">");
        return;

    case Step::DataCommand:
        if (code != 250 && code != 251) return failCurrent(QStringLiteral("SMTP: destinataire refuse %1").arg(code));
        ++m_rcptIndex;
        if (m_rcptIndex < m_config.to.size()) {
            writeLine("RCPT TO:<" + mailboxAddress(m_config.to.value(m_rcptIndex)).toUtf8() + ">");
            return;
        }
        m_step = Step::DataBody;
        writeLine("DATA");
        return;

    case Step::DataBody:
        if (code != 354) return failCurrent(QStringLiteral("SMTP: DATA refuse %1").arg(code));
        m_step = Step::Quit;
        m_socket->write(dotStuffed(m_current.data));
        m_socket->write("\r\n.\r\n");
        return;

    case Step::Quit:
        if (code != 250) return failCurrent(QStringLiteral("SMTP: message refuse %1").arg(code));
        writeLine("QUIT");
        finishCurrent();
        return;
    }
}

void EmailNotifier::advanceAfterEhlo() {
    if (!m_config.username.isEmpty()) {
        m_step = Step::AuthUser;
        writeLine("AUTH LOGIN");
        return;
    }
    m_step = Step::MailFrom;
    handleResponse(250, QByteArray());
}

QByteArray EmailNotifier::buildMessage(const Notification& notification) const {
    QString body;
    QTextStream stream(&body);
    stream << "Niveau : " << levelLabel(notification.level) << "\n";
    if (!notification.title.isEmpty())
        stream << "Source : " << notification.title << "\n";
    stream << "Date : " << QDateTime::fromSecsSinceEpoch(notification.ts).toString(Qt::ISODate) << "\n\n";
    stream << notification.message << "\n";

    QString message;
    QTextStream out(&message);
    out << "From: " << cleanHeader(m_config.from) << "\r\n";
    out << "To: " << cleanHeader(m_config.to.join(QStringLiteral(", "))) << "\r\n";
    out << "Subject: " << cleanHeader(subjectFor(notification)) << "\r\n";
    out << "MIME-Version: 1.0\r\n";
    out << "Content-Type: text/plain; charset=UTF-8\r\n";
    out << "Content-Transfer-Encoding: 8bit\r\n";
    out << "\r\n";
    out << body.replace("\n", "\r\n");
    return message.toUtf8();
}

QString EmailNotifier::subjectFor(const Notification& notification) const {
    QString subject = QStringLiteral("[%1]").arg(levelLabel(notification.level));
    if (!m_config.subjectPrefix.isEmpty())
        subject += QStringLiteral(" %1").arg(m_config.subjectPrefix);
    if (!notification.title.isEmpty())
        subject += QStringLiteral(" %1").arg(notification.title);
    return subject;
}

} // namespace morfnotify

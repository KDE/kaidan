// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LogHandler.h"

// Qt
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
// QXmpp
#include <QXmppClient.h>
// Kaidan
#include "Account.h"
#include "KaidanXmppLog.h"

LogHandler::LogHandler(AccountSettings *accountSettings, QXmppClient *client, QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
{
    auto *logger = new QXmppLogger(client);
    client->setLogger(logger);
    logger->setLoggingType(QXmppLogger::SignalLogging);
    logger->setPrettyXml(true);
    logger->setColorMode(QXmppLogger::ColorMode::ColorOn);
    connect(logger, &QXmppLogger::message, this, &LogHandler::handleLog);
}

void LogHandler::handleLog(QXmppLogger::MessageType type, const QString &text)
{
    switch (type) {
    case QXmppLogger::DebugMessage:
        qCDebug(KAIDAN_XMPP_LOG).noquote() << m_accountSettings->jid() << "[client] [debug]" << text;
        break;
    case QXmppLogger::ReceivedMessage:
        qCDebug(KAIDAN_XMPP_LOG).noquote() << m_accountSettings->jid() << "[incoming] <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
                                           << Qt::endl
                                           << text;
        break;
    case QXmppLogger::SentMessage:
        qCDebug(KAIDAN_XMPP_LOG).noquote() << m_accountSettings->jid() << "[outgoing] >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
                                           << Qt::endl
                                           << text;
        break;
    case QXmppLogger::WarningMessage:
        qCDebug(KAIDAN_XMPP_LOG).noquote() << m_accountSettings->jid() << "[client] [warn]" << text;
        break;
    default:
        qCDebug(KAIDAN_XMPP_LOG).noquote() << m_accountSettings->jid() << "[client] [various]" << text;
        break;
    }
}

#include "moc_LogHandler.cpp"

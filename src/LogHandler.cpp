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
#include "KaidanXmppLog.h"

LogHandler::LogHandler(QXmppClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    client->logger()->setLoggingType(QXmppLogger::SignalLogging);
}

void LogHandler::handleLog(QXmppLogger::MessageType type, const QString &text)
{
    switch (type) {
    case QXmppLogger::ReceivedMessage:
        qCDebug(KAIDAN_XMPP_LOG).noquote() << "[incoming] <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << Qt::endl
                                           << makeXmlPretty(text);
        break;
    case QXmppLogger::SentMessage:
        qCDebug(KAIDAN_XMPP_LOG).noquote() << "[outgoing] >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << Qt::endl
                                           << makeXmlPretty(text);
        break;
    case QXmppLogger::WarningMessage:
        qCDebug(KAIDAN_XMPP_LOG).noquote() << "[client] [warn]" << text;
        break;
    default:
        break;
    }
}

QString LogHandler::makeXmlPretty(QString xmlIn)
{
    QString xmlOut;

    QXmlStreamReader reader(xmlIn);
    QXmlStreamWriter writer(&xmlOut);
    writer.setAutoFormatting(true);

    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isWhitespace() && !reader.hasError()) {
            writer.writeCurrentToken(reader);
        }
    }

    // remove xml header
    xmlOut.replace(QStringLiteral("<?xml version=\"1.0\"?>"), QStringLiteral(""));

    // remove first & last char (\n)
    // first char is needed due to header replacement
    xmlOut = xmlOut.right(xmlOut.size() - 1);
    xmlOut = xmlOut.left(xmlOut.size() - 1);

    return xmlOut;
}

#include "moc_LogHandler.cpp"

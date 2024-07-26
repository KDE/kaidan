// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LogHandler.h"

// Qt
#include <QDebug>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
// QXmpp
#include <QXmppClient.h>
#include <QXmppLogger.h>

LogHandler::LogHandler(QXmppClient *client, bool enable, QObject *parent) : QObject(parent), m_client(client)
{
	client->logger()->setLoggingType(QXmppLogger::SignalLogging);
	enableLogging(enable);
}

void LogHandler::enableLogging(bool enabled)
{
	// check if we need to change something
	if (this->enabled == enabled)
		return;
	// update enabled status
	this->enabled = enabled;

	// apply change: enable or disable
	if (enabled)
		connect(m_client->logger(), &QXmppLogger::message, this, &LogHandler::handleLog);
	else
		disconnect(m_client->logger(), &QXmppLogger::message, this, &LogHandler::handleLog);
}

void LogHandler::handleLog(QXmppLogger::MessageType type, const QString &text)
{
	switch (type) {
	case QXmppLogger::ReceivedMessage:
		qDebug() << "[client] [incoming] <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
		qDebug().noquote() << makeXmlPretty(text);
		break;
	case QXmppLogger::SentMessage:
		qDebug() << "[client] [outgoing] >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
		qDebug().noquote() << makeXmlPretty(text);
		break;
	case QXmppLogger::WarningMessage:
		qDebug().noquote() << "[client] [warn]" << text;
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

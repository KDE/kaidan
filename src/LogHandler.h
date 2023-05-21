// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QXmppLogger.h>

class QXmppClient;

class LogHandler : public QObject
{
	Q_OBJECT

public:
	/**
	 * Default constructor
	 */
	LogHandler(QXmppClient *client, bool enable, QObject *parent = nullptr);

	/**
	 * Enable/disable logging to stdout (default: disabled)
	 */
	void enableLogging(bool enable);

	/**
	 * Handles logging messages and processes them (currently only output
	 * of XML streams)
	 */
	void handleLog(QXmppLogger::MessageType type, const QString &text);

private:
	/**
	 * Adds new lines to XML data and makes it more readable
	 */
	static QString makeXmlPretty(QString inputXml);

	QXmppClient *m_client;
	bool enabled = false;
};

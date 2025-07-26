// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
// QXmpp
#include <QXmppLogger.h>

class AccountSettings;
class QXmppClient;

class LogHandler : public QObject
{
    Q_OBJECT

public:
    LogHandler(AccountSettings *accountSettings, QXmppClient *client, QObject *parent = nullptr);

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

    AccountSettings *const m_accountSettings;
};

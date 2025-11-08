// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>

class AccountSettings;
class Connection;
class QXmppDiscoInfo;
class QXmppDiscoItem;
class QXmppDiscoveryManager;

class DiscoveryController : public QObject
{
public:
    DiscoveryController(AccountSettings *accountSettings, Connection *connection, QXmppDiscoveryManager *discoveryManager, QObject *parent = nullptr);

    ~DiscoveryController();

private:
    /**
     * Requests disco info and items from the server and updates the locally cached data.
     *
     * The results are used, for example, by various managers such as QXmppMixManager.
     */
    void updateData();

    void handleOwnServerInfo(QXmppDiscoInfo &&info);
    void handleOwnServerItems(QList<QXmppDiscoItem> &&items);

    AccountSettings *const m_accountSettings;
    QXmppDiscoveryManager *const m_manager;
};

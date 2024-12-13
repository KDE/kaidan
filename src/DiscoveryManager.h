// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
// QXmpp
#include <QXmppClient.h>
#include <QXmppDiscoveryIq.h>

class QXmppDiscoveryManager;

/**
 * @class DiscoveryManager Manager for outgoing/incoming service discovery requests and results
 *
 * XEP-0030: Service Discovery (https://xmpp.org/extensions/xep-0030.html)
 */
class DiscoveryManager : public QObject
{
public:
    DiscoveryManager(QXmppClient *client, QObject *parent = nullptr);

    ~DiscoveryManager();

private:
    /**
     * Requests disco info and items from the server.
     * The results are used, for example, by QXmppMixManager.
     */
    void requestData();

    /**
     * Handles incoming results of disco item requests
     */
    void handleItems(QList<QXmppDiscoveryIq::Item> &&);

    QXmppClient *m_client;
    QXmppDiscoveryManager *m_manager;
};

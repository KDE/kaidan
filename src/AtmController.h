// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>

class Database;
class QXmppClient;
class QXmppAtmManager;
class QXmppUri;
class TrustDb;

class AtmController : public QObject
{
    Q_OBJECT

public:
    AtmController(QObject *parent = nullptr);
    ~AtmController();

    /**
     * Sets the JID of the current account used to store the corresponding data
     * for a specific account.
     *
     * @param accountJid bare JID of the current account
     */
    void setAccountJid(const QString &accountJid);

    /**
     * Authenticates or distrusts end-to-end encryption keys by a given XMPP URI
     * (e.g., from a scanned QR code).
     *
     * @param uri Trust Message URI
     */
    void makeTrustDecisionsByUri(const QXmppUri &uri);
    Q_INVOKABLE void makeTrustDecisions(const QString &jid, const QList<QString> &keyIdsForAuthentication, const QList<QString> &keyIdsForDistrusting);

private:
    static QList<QByteArray> keyIdsFromHex(const QList<QString> &keyIds);

    QXmppAtmManager *const m_manager;
};

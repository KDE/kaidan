// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QFuture>
#include <QObject>
// QXmpp
#include <QXmppTrustLevel.h>

class QXmppAtmManager;

class AtmController : public QObject
{
    Q_OBJECT

public:
    /**
     * Result for making trust decisions by an XMPP URI specifying how the URI is used.
     */
    enum class TrustDecisionWithUriResult {
        MakingTrustDecisions, ///< The trust decisions are being made.
        JidUnexpected, ///< The URI's JID is not the expected one.
        InvalidUri, ///< The URI cannot be used for trust decisions.
    };
    Q_ENUM(TrustDecisionWithUriResult)

    explicit AtmController(QXmppAtmManager *manager, QObject *parent = nullptr);
    ~AtmController();

    /**
     * Sets the JID of the current account used to store the corresponding data
     * for a specific account.
     *
     * @param accountJid bare JID of the current account
     */
    void setAccountJid(const QString &accountJid);

    /**
     * Authenticates or distrusts end-to-end encryption keys with a given XMPP URI
     * (e.g., from a scanned QR code).
     *
     * Only if the URI's JID matches the expectedJid or no expectedJid is
     * passed, the trust decision is made.
     *
     * @param uriString string which can be an XMPP Trust Message URI
     * @param expectedJid JID of the key owner whose keys are expected to be
     *        authenticated or none to allow all JIDs
     */
    Q_INVOKABLE TrustDecisionWithUriResult makeTrustDecisionsWithUri(const QString &uriString, const QString &expectedJid = {});

    Q_INVOKABLE void makeTrustDecisions(const QString &jid, const QList<QString> &keyIdsForAuthentication, const QList<QString> &keyIdsForDistrusting);

    QFuture<QXmpp::TrustLevel> trustLevel(const QString &encryption, const QString &keyOwnerJid, const QByteArray &keyId);

private:
    static QList<QByteArray> keyIdsFromHex(const QList<QString> &keyIds);

    QXmppAtmManager *const m_manager;
};

Q_DECLARE_METATYPE(AtmController::TrustDecisionWithUriResult)

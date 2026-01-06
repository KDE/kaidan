// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2020 Andrea Scarpino <scarpino@kde.org>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "VCardController.h"

// Qt
#include <QBuffer>
#include <QTimer>
// QXmpp
#include <QXmppUtils.h>
#include <QXmppVCardIq.h>
#include <QXmppVCardManager.h>
// Kaidan
#include "Account.h"
#include "AvatarCache.h"
#include "FutureUtils.h"
#include "PresenceCache.h"
#include "RosterModel.h"

using namespace std::chrono_literals;

constexpr auto VCARD_FETCHING_AFTER_CONNECTING_DELAY = 500ms;

VCardController::VCardController(AccountSettings *accountSettings,
                                 Connection *connection,
                                 PresenceCache *presenceCache,
                                 QXmppClient *client,
                                 QXmppVCardManager *vCardManager,
                                 QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_connection(connection)
    , m_presenceCache(presenceCache)
    , m_manager(vCardManager)
{
    connect(client, &QXmppClient::presenceReceived, this, &VCardController::handlePresenceReceived);

    connect(m_manager, &QXmppVCardManager::vCardReceived, this, &VCardController::handleVCardReceived);
    connect(m_manager, &QXmppVCardManager::clientVCardReceived, this, &VCardController::handleOwnVCardReceived);

    // Get current nickname.
    connect(m_connection, &Connection::connected, this, &VCardController::requestOwnVCard);

    connect(RosterModel::instance(), &RosterModel::itemsFetched, this, &VCardController::handleRosterItemsFetched);
    connect(RosterModel::instance(), &RosterModel::itemAdded, this, &VCardController::handleRosterItemAdded);

    // Currently we're not requesting the own VCard on every connection because it is probably
    // way too resource intensive on mobile connections with many reconnects.
    // Actually we would need to request our own avatar, calculate the hash of it and publish
    // that in our presence.
    //
    // XEP-0084: User Avatar - probably best option (as long as the servers support XEP-0398:
    //                         User Avatar to vCard-Based Avatars Conversion)
}

void VCardController::requestVCard(const QString &jid)
{
    if (m_connection->state() == Enums::ConnectionState::StateConnected) {
        m_manager->requestVCard(jid);
    }
}

void VCardController::requestOwnVCard()
{
    if (m_connection->state() == Enums::ConnectionState::StateConnected) {
        m_manager->requestClientVCard();
    }
}

void VCardController::updateOwnVCard(const QXmppVCardIq &vCard)
{
    m_manager->setClientVCard(vCard);
}

void VCardController::changeNickname(const QString &nickname)
{
    m_nicknameToBeSetAfterReceivingCurrentVCard = nickname;
    requestOwnVCard();
}

void VCardController::changeAvatar(const QImage &avatar)
{
    // TODO Use a limit on the maximum avatar size
    if (!avatar.isNull()) {
        m_avatarToBeSetAfterReceivingCurrentVCard = avatar.scaledToWidth(512);
    } else {
        m_isAvatarToBeReset = true;
    }

    requestOwnVCard();
}

void VCardController::handleRosterItemsFetched(const QList<RosterItem> &rosterItems)
{
    for (const auto &rosterItem : rosterItems) {
        requestContactVCard(rosterItem.accountJid, rosterItem.jid, rosterItem.isGroupChat());
    }
}

void VCardController::handleRosterItemAdded(const RosterItem &rosterItem)
{
    requestContactVCard(rosterItem.accountJid, rosterItem.jid, rosterItem.isGroupChat());
}

void VCardController::requestContactVCard(const QString &accountJid, const QString &jid, bool isGroupChat)
{
    if (accountJid != m_accountSettings->jid() || isGroupChat) {
        return;
    }

    auto requestVCardOfUnavailableContact = [this, jid]() {
        QTimer::singleShot(VCARD_FETCHING_AFTER_CONNECTING_DELAY, this, [this, jid] {
            if (const auto presence = m_presenceCache->presence(jid); !presence || presence->type() == QXmppPresence::Unavailable) {
                requestVCard(jid);
            }
        });
    };

    if (m_connection->state() == Enums::ConnectionState::StateConnected) {
        requestVCardOfUnavailableContact();
    } else {
        connect(
            m_connection,
            &Connection::connected,
            this,
            [requestVCardOfUnavailableContact]() {
                requestVCardOfUnavailableContact();
            },
            Qt::SingleShotConnection);
    }
}

void VCardController::handleVCardReceived(const QXmppVCardIq &vCard)
{
    addAvatar(vCard);
    Q_EMIT vCardReceived(vCard);
}

void VCardController::handleOwnVCardReceived()
{
    if (!m_nicknameToBeSetAfterReceivingCurrentVCard.isNull()) {
        changeNicknameAfterReceivingCurrentVCard();
    }

    if (!m_avatarToBeSetAfterReceivingCurrentVCard.isNull() || m_isAvatarToBeReset) {
        changeAvatarAfterReceivingCurrentVCard();
    }

    const auto ownVCard = m_manager->clientVCard();
    Q_EMIT vCardReceived(ownVCard);

    if (m_nicknameToBeSetAfterReceivingCurrentVCard.isNull()) {
        m_accountSettings->setName(ownVCard.nickName());
    }
}

void VCardController::handlePresenceReceived(const QXmppPresence &presence)
{
    if (presence.vCardUpdateType() == QXmppPresence::VCardUpdateValidPhoto) {
        QString hash = AvatarCache::instance()->getHashOfJid(QXmppUtils::jidToBareJid(presence.from()));
        QString newHash = QString::fromUtf8(presence.photoHash().toHex());

        // check if hash differs and we need to refetch the avatar
        if (hash != newHash) {
            m_manager->requestVCard(QXmppUtils::jidToBareJid(presence.from()));
        }
    } else if (presence.vCardUpdateType() == QXmppPresence::VCardUpdateNoPhoto) {
        QString bareJid = QXmppUtils::jidToBareJid(presence.from());
        AvatarCache::instance()->clearAvatar(bareJid);
    }
    // ignore VCardUpdateNone (protocol unsupported) and VCardUpdateNotReady
}

void VCardController::changeNicknameAfterReceivingCurrentVCard()
{
    QXmppVCardIq vCard = m_manager->clientVCard();
    vCard.setNickName(m_nicknameToBeSetAfterReceivingCurrentVCard);
    m_manager->setClientVCard(vCard);
    m_nicknameToBeSetAfterReceivingCurrentVCard.clear();
}

void VCardController::changeAvatarAfterReceivingCurrentVCard()
{
    auto vCard = m_manager->clientVCard();

    if (m_isAvatarToBeReset) {
        m_isAvatarToBeReset = false;
        vCard.setPhoto({});
    } else {
        QByteArray avatar;
        QBuffer buffer(&avatar);
        buffer.open(QIODevice::WriteOnly);
        m_avatarToBeSetAfterReceivingCurrentVCard.save(&buffer, "JPG");
        vCard.setPhoto(avatar);
    }

    m_manager->setClientVCard(vCard);
    m_avatarToBeSetAfterReceivingCurrentVCard = {};
    Q_EMIT avatarChangeSucceeded();
}

void VCardController::addAvatar(const QXmppVCardIq &vCard)
{
    if (const auto avatar = vCard.photo(); !avatar.isEmpty()) {
        AvatarCache::instance()->addAvatar(QXmppUtils::jidToBareJid(vCard.from().isEmpty() ? m_accountSettings->jid() : vCard.from()), avatar);
    }
}

#include "moc_VCardController.cpp"

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
// QXmpp
#include <QXmppUtils.h>
#include <QXmppVCardIq.h>
#include <QXmppVCardManager.h>
// Kaidan
#include "AvatarFileStorage.h"
#include "FutureUtils.h"
#include "Kaidan.h"
#include "KaidanCoreLog.h"
#include "VCardCache.h"

VCardController::VCardController(QObject *parent)
    : QObject(parent)
    , m_clientWorker(Kaidan::instance()->client())
    , m_client(m_clientWorker->xmppClient())
    , m_manager(m_clientWorker->vCardManager())
    , m_avatarStorage(m_clientWorker->caches()->avatarStorage)
{
    connect(m_manager, &QXmppVCardManager::vCardReceived, this, &VCardController::handleVCardReceived);
    connect(m_client, &QXmppClient::presenceReceived, this, &VCardController::handlePresenceReceived);
    connect(m_manager, &QXmppVCardManager::clientVCardReceived, this, &VCardController::handleClientVCardReceived);

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
    if (Kaidan::instance()->connectionState() == Enums::ConnectionState::StateConnected) {
        runOnThread(m_manager, [this, jid]() {
            m_manager->requestVCard(jid);
        });
    } else {
        qCWarning(KAIDAN_CORE_LOG) << "Could not fetch vCard: Not connected to server";
    }
}

void VCardController::requestClientVCard()
{
    if (Kaidan::instance()->connectionState() == Enums::ConnectionState::StateConnected) {
        runOnThread(m_manager, [this]() {
            m_manager->requestClientVCard();
        });
    } else {
        qCWarning(KAIDAN_CORE_LOG) << "Could not fetch own vCard: Not connected to server";
    }
}

void VCardController::changeNickname(const QString &nickname)
{
    runOnThread(m_clientWorker, [this, nickname]() {
        m_clientWorker->startTask([this, nickname] {
            runOnThread(this, [this, nickname]() {
                m_nicknameToBeSetAfterReceivingCurrentVCard = nickname;
                requestClientVCard();
            });
        });
    });
}

void VCardController::changeAvatar(const QImage &avatar)
{
    runOnThread(m_clientWorker, [this, avatar]() {
        m_clientWorker->startTask([this, avatar] {
            runOnThread(this, [this, avatar]() {
                // TODO Use a limit on the maximum avatar size
                if (!avatar.isNull()) {
                    m_avatarToBeSetAfterReceivingCurrentVCard = avatar.scaledToWidth(512);
                } else {
                    m_isAvatarToBeReset = true;
                }

                requestClientVCard();
            });
        });
    });
}

void VCardController::handleVCardReceived(const QXmppVCardIq &iq)
{
    if (!iq.photo().isEmpty()) {
        m_avatarStorage->addAvatar(QXmppUtils::jidToBareJid(iq.from().isEmpty() ? m_client->configuration().jid() : iq.from()), iq.photo());
    }

    Q_EMIT vCardReceived(iq);
}

void VCardController::handleClientVCardReceived()
{
    if (!m_nicknameToBeSetAfterReceivingCurrentVCard.isNull()) {
        changeNicknameAfterReceivingCurrentVCard();
    }

    if (!m_avatarToBeSetAfterReceivingCurrentVCard.isNull() || m_isAvatarToBeReset) {
        changeAvatarAfterReceivingCurrentVCard();
    }

    runOnThread(
        m_clientWorker,
        [this]() {
            return std::tuple{m_client->configuration().jidBare(), m_manager->clientVCard()};
        },
        this,
        [this](std::tuple<QString, QXmppVCardIq> &&result) {
            auto [accountJid, clientVCard] = result;
            clientVCard.setFrom(accountJid);

            m_clientWorker->caches()->vCardCache->setVCard(accountJid, clientVCard);
        });
}

void VCardController::handlePresenceReceived(const QXmppPresence &presence)
{
    if (presence.vCardUpdateType() == QXmppPresence::VCardUpdateValidPhoto) {
        QString hash = m_avatarStorage->getHashOfJid(QXmppUtils::jidToBareJid(presence.from()));
        QString newHash = QString::fromUtf8(presence.photoHash().toHex());

        // check if hash differs and we need to refetch the avatar
        if (hash != newHash) {
            runOnThread(m_manager, [this, presence]() {
                m_manager->requestVCard(QXmppUtils::jidToBareJid(presence.from()));
            });
        }
    } else if (presence.vCardUpdateType() == QXmppPresence::VCardUpdateNoPhoto) {
        QString bareJid = QXmppUtils::jidToBareJid(presence.from());
        m_avatarStorage->clearAvatar(bareJid);
    }
    // ignore VCardUpdateNone (protocol unsupported) and VCardUpdateNotReady
}

void VCardController::changeNicknameAfterReceivingCurrentVCard()
{
    runOnThread(
        m_manager,
        [this, nickname = m_nicknameToBeSetAfterReceivingCurrentVCard]() {
            QXmppVCardIq vCardIq = m_manager->clientVCard();
            vCardIq.setNickName(nickname);
            m_manager->setClientVCard(vCardIq);
        },
        this,
        [this]() {
            m_nicknameToBeSetAfterReceivingCurrentVCard.clear();

            runOnThread(m_clientWorker, [this] {
                m_clientWorker->finishTask();
            });
        });
}

void VCardController::changeAvatarAfterReceivingCurrentVCard()
{
    runOnThread(
        m_manager,
        [this]() {
            return m_manager->clientVCard();
        },
        this,
        [this](QXmppVCardIq &&vCardIq) {
            if (!m_isAvatarToBeReset) {
                QByteArray ba;
                QBuffer buffer(&ba);
                buffer.open(QIODevice::WriteOnly);
                m_avatarToBeSetAfterReceivingCurrentVCard.save(&buffer, "JPG");

                vCardIq.setPhoto(ba);
            } else {
                m_isAvatarToBeReset = false;
                vCardIq.setPhoto({});
            }

            runOnThread(
                m_manager,
                [this, vCardIq]() {
                    m_manager->setClientVCard(vCardIq);
                },
                this,
                [this]() {
                    m_avatarToBeSetAfterReceivingCurrentVCard = {};

                    runOnThread(m_clientWorker, [this]() {
                        m_clientWorker->finishTask();
                        Q_EMIT avatarChangeSucceeded();
                    });
                });
        });
}

#include "moc_VCardController.cpp"

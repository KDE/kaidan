/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MessageModel.h"

#include <chrono>

// Qt
#include <QGuiApplication>
#include <QTimer>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "AccountManager.h"
#include "FutureUtils.h"
#include "Kaidan.h"
#include "MessageDb.h"
#include "MessageHandler.h"
#include "Notifications.h"
#include "OmemoManager.h"
#include "QmlUtils.h"
#include "RosterModel.h"
#include "RosterItemWatcher.h"

using namespace std::chrono_literals;

constexpr auto PAUSED_TYPING_TIMEOUT = 10s;
constexpr auto ACTIVE_TIMEOUT = 2min;
constexpr auto TYPING_TIMEOUT = 2s;

// defines that the message is suitable for correction only if it is among the N latest messages
constexpr int MAX_CORRECTION_MESSAGE_COUNT_DEPTH = 20;
// defines that the message is suitable for correction only if it has ben sent not earlier than N days ago
constexpr int MAX_CORRECTION_MESSAGE_DAYS_DEPTH = 2;

MessageModel *MessageModel::s_instance = nullptr;

MessageModel *MessageModel::instance()
{
	return s_instance;
}

MessageModel::MessageModel(QObject *parent)
	: QAbstractListModel(parent),
	  m_composingTimer(new QTimer(this)),
	  m_stateTimeoutTimer(new QTimer(this)),
	  m_inactiveTimer(new QTimer(this)),
	  m_chatPartnerChatStateTimeout(new QTimer(this))
{
	Q_ASSERT(!s_instance);
	s_instance = this;

	connect(this, &MessageModel::keysRetrieved, this, &MessageModel::handleKeysRetrieved);
	connect(this, &MessageModel::encryptionChanged, this, &MessageModel::isOmemoEncryptionEnabledChanged);
	connect(this, &MessageModel::usableOmemoDevicesChanged, this, &MessageModel::isOmemoEncryptionEnabledChanged);
	connect(&m_rosterItemWatcher, &RosterItemWatcher::itemChanged, this, &MessageModel::isOmemoEncryptionEnabledChanged);

	connect(&m_accountOmemoWatcher, &OmemoWatcher::usableOmemoDevicesChanged, this, &MessageModel::usableOmemoDevicesChanged);
	connect(&m_contactOmemoWatcher, &OmemoWatcher::usableOmemoDevicesChanged, this, &MessageModel::usableOmemoDevicesChanged);

	// Timer to set state to paused
	m_composingTimer->setSingleShot(true);
	m_composingTimer->setInterval(TYPING_TIMEOUT);
	m_composingTimer->callOnTimeout(this, [this] {
		sendChatState(QXmppMessage::Paused);

		// 10 seconds after user stopped typing, remove "paused" state
		m_stateTimeoutTimer->start(PAUSED_TYPING_TIMEOUT);
	});

	// Timer to reset typing-related notifications like paused and composing to active
	m_stateTimeoutTimer->setSingleShot(true);
	m_stateTimeoutTimer->callOnTimeout(this, [this] {
		sendChatState(QXmppMessage::Active);
	});

	// Timer to time out active state
	m_inactiveTimer->setSingleShot(true);
	m_inactiveTimer->setInterval(ACTIVE_TIMEOUT);
	m_inactiveTimer->callOnTimeout(this, [this] {
		sendChatState(QXmppMessage::Inactive);
	});

	// Timer to reset the chat partners state
	// if they lost connection while a state other then gone was active
	m_chatPartnerChatStateTimeout->setSingleShot(true);
	m_chatPartnerChatStateTimeout->setInterval(ACTIVE_TIMEOUT);
	m_chatPartnerChatStateTimeout->callOnTimeout(this, [this] {
		m_chatPartnerChatState = QXmppMessage::Gone;
		m_chatStateCache.insert(m_currentChatJid, QXmppMessage::Gone);
		emit chatStateChanged();
	});

	connect(MessageDb::instance(), &MessageDb::messagesFetched,
	        this, &MessageModel::handleMessagesFetched);
	connect(MessageDb::instance(), &MessageDb::pendingMessagesFetched,
	        this, &MessageModel::pendingMessagesFetched);

	// addMessage requests are forwarded to the MessageDb, are deduplicated there and
	// added if MessageDb::messageAdded is emitted
	connect(MessageDb::instance(), &MessageDb::messageAdded, this, &MessageModel::handleMessage);

	connect(MessageDb::instance(), &MessageDb::messageUpdated, this, &MessageModel::updateMessage);

	connect(this, &MessageModel::handleChatStateRequested,
	        this, &MessageModel::handleChatState);

	connect(this, &MessageModel::removeMessagesRequested, this, &MessageModel::removeMessages);
	connect(this, &MessageModel::removeMessagesRequested, MessageDb::instance(), &MessageDb::removeMessages);

	connect(this, &MessageModel::mamBacklogRetrieved, this, &MessageModel::handleMamBacklogRetrieved);
}

MessageModel::~MessageModel() = default;

bool MessageModel::isEmpty() const
{
	return m_messages.isEmpty();
}

int MessageModel::rowCount(const QModelIndex &) const
{
	return m_messages.length();
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[Timestamp] = "timestamp";
	roles[Id] = "id";
	roles[Sender] = "sender";
	roles[Recipient] = "recipient";
	roles[Encryption] = "encryption";
	roles[IsTrusted] = "isTrusted";
	roles[Body] = "body";
	roles[IsOwn] = "isOwn";
	roles[IsEdited] = "isEdited";
	roles[DeliveryState] = "deliveryState";
	roles[IsLastRead] = "isLastRead";
	roles[IsSpoiler] = "isSpoiler";
	roles[SpoilerHint] = "spoilerHint";
	roles[ErrorText] = "errorText";
	roles[DeliveryStateIcon] = "deliveryStateIcon";
	roles[DeliveryStateName] = "deliveryStateName";
	roles[Files] = "files";
	roles[Reactions] = "reactions";
	return roles;
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
	if (!hasIndex(index.row(), index.column(), index.parent())) {
		qWarning() << "Could not get data from message model." << index << role;
		return {};
	}
	const Message &msg = m_messages.at(index.row());

	switch (role) {
	case Timestamp:
		return msg.stamp;
	case Id:
		return msg.id;
	case Sender:
		return msg.from;
	case Recipient:
		return msg.to;
	case Encryption:
		return msg.encryption;
	case IsTrusted: {
		if (msg.isOwn && msg.senderKey.isEmpty()) {
			return true;
		}

		const auto trustLevel = m_keys.value(msg.from).value(msg.senderKey);
		return (QXmpp::TrustLevel::AutomaticallyTrusted | QXmpp::TrustLevel::ManuallyTrusted | QXmpp::TrustLevel::Authenticated).testFlag(trustLevel);
	}
	case Body:
		return msg.body;
	case IsOwn:
		return msg.isOwn;
	case IsEdited:
		return msg.isEdited;
	case IsLastRead:
		// A read marker text is only displayed if the message is the last read message and no
		// message is received by the contact after it.
		if (msg.id == m_lastReadOwnMessageId) {
			for (auto i = index.row(); i >= 0; --i) {
				if (m_messages.at(i).from != m_currentAccountJid) {
					return false;
				}
			}
			return true;
		}
		return false;
	case DeliveryState:
		return QVariant::fromValue(msg.deliveryState);
	case IsSpoiler:
		return msg.isSpoiler;
	case SpoilerHint:
		return msg.spoilerHint;
	case ErrorText:
		return msg.errorText;
	case DeliveryStateIcon:
		switch (msg.deliveryState) {
		case DeliveryState::Pending:
			return QmlUtils::getResourcePath("images/dots.svg");
		case DeliveryState::Sent:
			return QmlUtils::getResourcePath("images/check-mark-pale.svg");
		case DeliveryState::Delivered:
			return QmlUtils::getResourcePath("images/check-mark.svg");
		case DeliveryState::Error:
			return QmlUtils::getResourcePath("images/cross.svg");
		case DeliveryState::Draft:
			Q_UNREACHABLE();
		}
		return {};
	case DeliveryStateName:
		switch (msg.deliveryState) {
		case DeliveryState::Pending:
			return tr("Pending");
		case DeliveryState::Sent:
			return tr("Sent");
		case DeliveryState::Delivered:
			return tr("Delivered");
		case DeliveryState::Error:
			return tr("Error");
		case DeliveryState::Draft:
			Q_UNREACHABLE();
		}
		return {};
	case Files:
		return QVariant::fromValue(msg.files);
	case Reactions:
		// emojis mapped to the JIDs of their senders
		QMap<QString, QVector<QString>> reactionsMap;

		// Create an appropriate mapping for the user interface.
		const auto &reactions = msg.reactions;
		for (auto itr = reactions.begin(); itr != reactions.end(); ++itr) {
			for (const auto &emoji : std::as_const(itr->emojis)) {
				auto &senderJids = reactionsMap[emoji];
				senderJids.append(itr.key());
				std::sort(senderJids.begin(), senderJids.end());
			}
		}

		// TODO: Find better way to create a QVariantMap to be returned
		QVariantMap reactionsVariant;
		for (auto itr = reactionsMap.begin(); itr != reactionsMap.end(); ++itr) {
			reactionsVariant.insert(itr.key(), { QVariant::fromValue(itr.value()) });
		}

		return reactionsVariant;
	}
	return {};
}

void MessageModel::fetchMore(const QModelIndex &)
{
	if (!m_fetchedAllFromDb) {
		if (m_messages.isEmpty()) {
			// If there are unread messages, all messages until the first unread message are
			// fetched.
			// Otherwise, the messages are fetched by their regular limit.
			if (m_rosterItemWatcher.item().unreadMessages > 0) {
				const auto lastReadContactMessageId = m_rosterItemWatcher.item().lastReadContactMessageId;

				// lastReadContactMessageId can be empty if there is no contact message stored or
				// the oldest stored contact message is marked as first unread.
				if (lastReadContactMessageId.isEmpty()) {
					MessageDb::instance()->fetchMessagesUntilFirstContactMessage(
							AccountManager::instance()->jid(), m_currentChatJid, 0);
				} else {
					MessageDb::instance()->fetchMessagesUntilId(
							AccountManager::instance()->jid(), m_currentChatJid, 0, lastReadContactMessageId);
				}
			} else {
				MessageDb::instance()->fetchMessages(
						AccountManager::instance()->jid(), m_currentChatJid, 0);
			}
		} else {
			MessageDb::instance()->fetchMessages(
					AccountManager::instance()->jid(), m_currentChatJid, m_messages.size());
		}
	} else if (!m_fetchedAllFromMam) {
		// use earliest timestamp
		const auto lastStamp = [this]() -> QDateTime {
			const auto stamp1 = m_mamBacklogLastStamp.isNull() ? QDateTime::currentDateTimeUtc() : m_mamBacklogLastStamp;
			if (!m_messages.empty()) {
				return std::min(stamp1, m_messages.constLast().stamp);
			}
			return stamp1;
		};

		// Skip unneeded steps when 'canFetchMore()' has not been called before calling
		// 'fetchMore()'.
		if (!m_mamLoading) {
			emit Kaidan::instance()->client()->messageHandler()->retrieveBacklogMessagesRequested(m_currentChatJid, lastStamp());
			setMamLoading(true);
		}
	}
	// already fetched everything from DB and MAM
}

bool MessageModel::canFetchMore(const QModelIndex &) const
{
	return !m_fetchedAllFromDb || (!m_fetchedAllFromMam && !m_mamLoading);
}

QString MessageModel::currentAccountJid()
{
	return m_currentAccountJid;
}

QString MessageModel::currentChatJid()
{
	return m_currentChatJid;
}

void MessageModel::setCurrentChat(const QString &accountJid, const QString &chatJid)
{
	if (accountJid == m_currentAccountJid && chatJid == m_currentChatJid) {
		return;
	}

	resetCurrentChat(accountJid, chatJid);

	// Send "active" state for the current chat.
	sendChatState(QXmppMessage::State::Active);

	runOnThread(Kaidan::instance()->client()->omemoManager(), [accountJid, chatJid] {
		Kaidan::instance()->client()->omemoManager()->initializeChat(accountJid, chatJid);
	});
}

void MessageModel::resetCurrentChat() {
	resetCurrentChat({}, {});
}

QString MessageModel::currentDraftMessageId() const
{
	return m_rosterItemWatcher.item().draftMessageId;
}

bool MessageModel::isChatCurrentChat(const QString &accountJid, const QString &chatJid) const
{
	return accountJid == m_currentAccountJid && chatJid == m_currentChatJid;
}

QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> MessageModel::keys()
{
	return m_keys;
}

Encryption::Enum MessageModel::activeEncryption()
{
	return isOmemoEncryptionEnabled() ? Encryption::Omemo2 : Encryption::NoEncryption;
}

bool MessageModel::isOmemoEncryptionEnabled() const
{
	return encryption() == Encryption::Omemo2 && !usableOmemoDevices().isEmpty();
}

Encryption::Enum MessageModel::encryption() const
{
	return RosterModel::instance()->itemEncryption(m_currentAccountJid, m_currentChatJid)
			.value_or(Encryption::NoEncryption);
}

void MessageModel::setEncryption(Encryption::Enum encryption)
{
	RosterModel::instance()->setItemEncryption(m_currentAccountJid, m_currentChatJid, encryption);
	emit encryptionChanged();
}

QList<QString> MessageModel::usableOmemoDevices() const
{
	   return m_currentAccountJid == m_currentChatJid ? m_accountOmemoWatcher.usableOmemoDevices() : m_contactOmemoWatcher.usableOmemoDevices();
}

void MessageModel::resetComposingChatState()
{
	m_composingTimer->stop();
	m_stateTimeoutTimer->stop();

	// Reset composing chat state after message is sent
	sendChatState(QXmppMessage::State::Active);
}

void MessageModel::handleMessageRead(int readMessageIndex)
{
	// Check the index validity.
	if (readMessageIndex < 0 || readMessageIndex >= m_messages.size()) {
		return;
	}

	const auto &lastReadContactMessageId = m_rosterItemWatcher.item().lastReadContactMessageId;

	// Skip messages that are read but older than the last read message.
	for (int i = 0; i != m_messages.size(); ++i) {
		if (m_messages.at(i).id == lastReadContactMessageId && i <= readMessageIndex) {
			return;
		}
	}

	Message readContactMessage;
	int readContactMessageIndex;

	// Search for the last read contact message if it is at the top of the chat page list view but
	// readMessageIndex is of an own message.
	if (const auto &readMessage = m_messages.at(readMessageIndex); readMessage.isOwn) {
		auto isContactMessageRead = false;

		for (int i = readMessageIndex + 1; i != m_messages.size(); ++i) {
			if (const auto &message = m_messages.at(i); !message.isOwn) {
				readContactMessageIndex = i;
				readContactMessage = message;
				isContactMessageRead = true;
				break;
			}
		}

		// Skip the remaining processing if no contact message is read.
		if (!isContactMessageRead) {
			return;
		}
	} else {
		readContactMessageIndex = readMessageIndex;
		readContactMessage = readMessage;
	}

	const auto readMessageId = readContactMessage.id;
	const auto isApplicationActive = QGuiApplication::applicationState() == Qt::ApplicationActive;

	if (lastReadContactMessageId != readMessageId && isApplicationActive) {
		Notifications::instance()->closeMessageNotification(m_currentAccountJid, m_currentChatJid);

		bool readMarkerPending = true;
		if (Enums::ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
			if (m_rosterItemWatcher.item().readMarkerSendingEnabled) {
				runOnThread(Kaidan::instance()->client()->messageHandler(), [chatJid = m_currentChatJid, readMessageId]() {
					Kaidan::instance()->client()->messageHandler()->sendReadMarker(chatJid, readMessageId);
				});
			}

			readMarkerPending = false;
		}

		emit RosterModel::instance()->updateItemRequested(m_currentChatJid, [=, this](RosterItem &item) {
			item.lastReadContactMessageId = readMessageId;
			item.readMarkerPending = readMarkerPending;

			// If the read message is the latest one or lastReadContactMessageId is empty, reset the
			// counter for unread messages.
			// lastReadContactMessageId can be empty if there is no contact message stored or the
			// oldest stored contact message is marked as first unread.
			// Otherwise, decrease it by the number of contact messages between the read contact
			// message and the last read contact message.
			if (readContactMessageIndex == 0 || lastReadContactMessageId.isEmpty()) {
				item.unreadMessages = 0;
			} else {
				int readMessageCount = 1;
				for (int i = readContactMessageIndex + 1; i < m_messages.size(); ++i) {
					if (const auto &message = m_messages.at(i); message.id == lastReadContactMessageId) {
						break;
					} else if (!message.isOwn) {
						++readMessageCount;
					}
				}

				item.unreadMessages = item.unreadMessages - readMessageCount;
			}
		});
	}
}

int MessageModel::firstUnreadContactMessageIndex()
{
	const auto lastReadContactMessageId = m_rosterItemWatcher.item().lastReadContactMessageId;
	int lastReadContactMessageIndex = -1;
	for (auto i = 0; i < m_messages.size(); ++i) {
		const auto &message = m_messages.at(i);
		const auto &messageId = message.id;

		// lastReadContactMessageId can be empty if there is no contact message stored or the oldest
		// stored contact message is marked as first unread.
		if (!message.isOwn && (messageId == lastReadContactMessageId || lastReadContactMessageId.isEmpty())) {
			lastReadContactMessageIndex = i;
		}
	}

	if (lastReadContactMessageIndex > 0) {
		for (auto i = lastReadContactMessageIndex - 1; i >= 0; --i) {
			if (!m_messages.at(i).isOwn) {
				return i;
			}
		}
	}

	return lastReadContactMessageIndex;
}

void MessageModel::markMessageAsFirstUnread(int index)
{
	int unreadMessageCount = 1;
	QString lastReadContactMessageId;

	// Determine the count of unread contact messages before the marked one.
	for (int i = 0; i != index; ++i) {
		if (!m_messages.at(i).isOwn) {
			++unreadMessageCount;
		}
	}

	// Search for the first contact message after the marked one.
	if (index < m_messages.size() - 1) {
		for (int i = index + 1; i != m_messages.size(); ++i) {
			const auto &message = m_messages.at(i);
			if (!message.isOwn) {
				lastReadContactMessageId = message.id;
				break;
			}
		}
	}

	// Find the last read contact message in the database in order to update the last read contact
	// message ID.
	// That is needed if a message is marked as the first unread message while the previous message
	// of the contact is not fetched from the database and thus not in m_messages.
	if (lastReadContactMessageId.isEmpty()) {
		auto future = MessageDb::instance()->firstContactMessageId(m_currentAccountJid, m_currentChatJid, unreadMessageCount);
		await(future, this, [=, currentChatJid = m_currentChatJid](QString firstContactMessageId) {
			emit RosterModel::instance()->updateItemRequested(currentChatJid, [=](RosterItem &item) {
				item.unreadMessages = unreadMessageCount;
				item.lastReadContactMessageId = firstContactMessageId;
			});
		});
	} else  {
		emit RosterModel::instance()->updateItemRequested(m_currentChatJid, [=](RosterItem &item) {
			item.unreadMessages = unreadMessageCount;
			item.lastReadContactMessageId = lastReadContactMessageId;
		});
	}
}

void MessageModel::addMessageReaction(const QString &messageId, const QString &emoji)
{
	const auto itr = std::find_if(m_messages.begin(), m_messages.end(), [&](const Message &message) {
		return message.id == messageId;
	});

	// Add a message reaction if the corresponding message could be found and the reaction is no
	// duplicate.
	if (itr != m_messages.end()) {
		if (auto emojis = itr->reactions.value(m_currentAccountJid).emojis; !emojis.contains(emoji)) {
			MessageDb::instance()->updateMessage(messageId, [this, emoji](Message &message) {
				auto &reaction = message.reactions[m_currentAccountJid];
				reaction.latestTimestamp = QDateTime::currentDateTimeUtc();
				reaction.emojis.append(emoji);
			});

			emojis.append(emoji);
			runOnThread(Kaidan::instance()->client()->messageHandler(), [this, messageId, emojis] {
				Kaidan::instance()->client()->messageHandler()->sendMessageReaction(m_currentChatJid, messageId, emojis);
			});
		}
	}
}

void MessageModel::removeMessageReaction(const QString &messageId, const QString &emoji)
{
	const auto itr = std::find_if(m_messages.begin(), m_messages.end(), [&](const Message &message) {
		return message.id == messageId;
	});

	if (itr != m_messages.end()) {
		MessageDb::instance()->updateMessage(messageId, [this, emoji](Message &message) {
			auto &reactions = message.reactions;

			auto &reaction = reactions[m_currentAccountJid];
			reaction.latestTimestamp = QDateTime::currentDateTimeUtc();

			auto &emojis = reaction.emojis;
			emojis.removeOne(emoji);

			// Remove the reaction if it has no emojis anymore.
			if (emojis.isEmpty()) {
				reactions.remove(m_currentAccountJid);
			}
		});

		auto emojis = itr->reactions[m_currentAccountJid].emojis;
		emojis.removeOne(emoji);
		runOnThread(Kaidan::instance()->client()->messageHandler(), [this, messageId, emojis] {
			Kaidan::instance()->client()->messageHandler()->sendMessageReaction(m_currentChatJid, messageId, emojis);
		});
	}
}

bool MessageModel::canCorrectMessage(int index) const
{
	// check index validity
	if (index < 0 || index >= m_messages.size())
		return false;

	// message needs to be sent by us and needs to be no error message
	const auto &msg = m_messages.at(index);
	if (!msg.isOwn || msg.deliveryState == Enums::DeliveryState::Error)
		return false;

	// check time limit
	const auto timeThreshold =
		QDateTime::currentDateTimeUtc().addDays(-MAX_CORRECTION_MESSAGE_DAYS_DEPTH);
	if (msg.stamp < timeThreshold)
		return false;

	// check messages count limit
	for (int i = 0, count = 0; i < index; i++) {
		if (m_messages.at(i).isOwn && ++count == MAX_CORRECTION_MESSAGE_COUNT_DEPTH)
			return false;
	}

	return true;
}

void MessageModel::handleMessagesFetched(const QVector<Message> &msgs)
{
	if (msgs.length() < DB_QUERY_LIMIT_MESSAGES)
		m_fetchedAllFromDb = true;

	if (msgs.empty()) {
		// If nothing can be retrieved from the DB, directly try MAM instead.
		if (m_fetchedAllFromDb) {
			fetchMore({});
		} else {
			emit messageFetchingFinished();
		}

		return;
	}

	beginInsertRows(QModelIndex(), rowCount(), rowCount() + msgs.length() - 1);
	for (auto msg : msgs) {
		// Skip messages that were not fetched for this chat
		if (msg.from != m_currentChatJid && msg.to != m_currentChatJid) {
			continue;
		}
		msg.isOwn = AccountManager::instance()->jid() == msg.from;
		processMessage(msg);
		m_messages << msg;
	}
	endInsertRows();

	emit messageFetchingFinished();
}

void MessageModel::handleMamBacklogRetrieved(const QString &accountJid, const QString &jid, const QDateTime &lastStamp, bool complete)
{
	if (m_currentAccountJid == accountJid && m_currentChatJid == jid) {
		// The stamp is required for the following scenario (that already happened to me).
		// The full count of messages is requested and returned, but no message has a body
		// and so no new message is added to the MessageModel. The MessageModel then tries
		// to load the same messages over and over again.
		// Solution: Cache the last stamp from the query and request messages older than
		// that
		m_mamBacklogLastStamp = lastStamp;
		setMamLoading(false);
		if (complete) {
			m_fetchedAllFromMam = true;
		}

		if (m_rosterItemWatcher.item().lastReadContactMessageId.isEmpty()) {
			for (const auto &message : std::as_const(m_messages)) {
				if (!message.isOwn) {
					emit RosterModel::instance()->updateItemRequested(m_currentChatJid, [=, messageId = message.id](RosterItem &item) {
						item.lastReadContactMessageId = messageId;
					});
					break;
				}
			}
		}

		emit messageFetchingFinished();
	}
}

void MessageModel::resetCurrentChat(const QString &accountJid, const QString &chatJid)
{
	// Send "gone" state for the previous chat.
	if (!m_currentAccountJid.isEmpty() && !m_currentChatJid.isEmpty()) {
		sendChatState(QXmppMessage::State::Gone);
	}

	// Setting of the following attributes must be done before sending chat states for the new chat.
	// Otherwise, the chat states would be sent for the previous chat.
	m_currentAccountJid = accountJid;
	m_currentChatJid = chatJid;
	emit currentAccountJidChanged(accountJid);
	emit currentChatJidChanged(chatJid);

	m_rosterItemWatcher.setJid(chatJid);
	m_accountOmemoWatcher.setJid(accountJid);
	m_contactOmemoWatcher.setJid(chatJid);
	m_lastReadOwnMessageId = m_rosterItemWatcher.item().lastReadOwnMessageId;
	emit currentDraftMessageIdChanged(currentDraftMessageId());

	// Reset the chat states of the previous chat.
	m_ownChatState = QXmppMessage::State::None;
	m_chatPartnerChatState = m_chatStateCache.value(chatJid, QXmppMessage::State::Gone);
	m_composingTimer->stop();
	m_stateTimeoutTimer->stop();
	m_inactiveTimer->stop();
	m_chatPartnerChatStateTimeout->stop();

	removeAllMessages();
}

void MessageModel::removeMessages(const QString &accountJid, const QString &chatJid)
{
	if (accountJid == m_currentAccountJid && chatJid == m_currentChatJid)
		removeAllMessages();
}

void MessageModel::removeAllMessages()
{
	if (!m_messages.isEmpty()) {
		beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
		m_messages.clear();
		endRemoveRows();
	}

	m_fetchedAllFromDb = false;
	m_fetchedAllFromMam = false;
	m_mamBacklogLastStamp = QDateTime();
	setMamLoading(false);
}

void MessageModel::insertMessage(int idx, const Message &msg)
{
	beginInsertRows(QModelIndex(), idx, idx);
	m_messages.insert(idx, msg);
	endInsertRows();

	updateLastReadOwnMessageId();
}

void MessageModel::addMessage(const Message &msg)
{
	// index where to add the new message
	int i = 0;
	for (const auto &message : qAsConst(m_messages)) {
		if (msg.stamp > message.stamp) {
			insertMessage(i, msg);
			return;
		}
		i++;
	}
	// add message to the end of the list
	insertMessage(i, msg);
}

void MessageModel::updateMessage(const QString &id,
                                 const std::function<void(Message &)> &updateMsg)
{
	for (int i = 0; i < m_messages.length(); i++) {
		if (m_messages.at(i).id == id) {
			// update message
			Message msg = m_messages.at(i);
			updateMsg(msg);

			// check if item was actually modified
			if (m_messages.at(i) == msg)
				return;

			// check, if the position of the new message may be different
			if (msg.stamp == m_messages.at(i).stamp) {
				beginRemoveRows(QModelIndex(), i, i);
				m_messages.removeAt(i);
				endRemoveRows();

				// add the message at the same position
				insertMessage(i, msg);
			} else {
				beginRemoveRows(QModelIndex(), i, i);
				m_messages.removeAt(i);
				endRemoveRows();

				// put to new position
				addMessage(msg);
			}

			showMessageNotification(msg, MessageOrigin::Stream);

			break;
		}
	}
}

void MessageModel::updateLastReadOwnMessageId()
{
	const auto formerLastReadOwnMessageId = m_lastReadOwnMessageId;
	m_lastReadOwnMessageId = m_rosterItemWatcher.item().lastReadOwnMessageId;

	int formerLastReadOwnMessageIndex = -1;
	int lastReadOwnMessageIndex = -1;

	// The message that was the former last read message and the message that is the last read
	// message now need to be updated for the user interface in order to reflect the most recent
	// state.
	for (int i = 0; i != m_messages.size(); ++i) {
		const auto &message = m_messages.at(i);
		if (message.id == formerLastReadOwnMessageId) {
			formerLastReadOwnMessageIndex = i;

			const auto modelIndex = index(formerLastReadOwnMessageIndex);
			emit dataChanged(modelIndex, modelIndex, { IsLastRead });

			if (lastReadOwnMessageIndex != -1) {
				break;
			}
		} else if (message.id == m_lastReadOwnMessageId) {
			lastReadOwnMessageIndex = i;

			const auto modelIndex = index(lastReadOwnMessageIndex);
			emit dataChanged(modelIndex, modelIndex, { IsLastRead });

			if (formerLastReadOwnMessageIndex != -1) {
				break;
			}
		}
	}
}

void MessageModel::handleMessage(Message msg, MessageOrigin origin)
{
	processMessage(msg);

	showMessageNotification(msg, origin);

	if (msg.from == m_currentChatJid || msg.to == m_currentChatJid) {
		addMessage(std::move(msg));
	}
}

int MessageModel::searchForMessageFromNewToOld(const QString &searchString, int startIndex)
{
	int foundIndex = startIndex;

	if (foundIndex < m_messages.size()) {
		for (; foundIndex < m_messages.size(); foundIndex++) {
			if (m_messages.at(foundIndex).body.contains(searchString, Qt::CaseInsensitive)) {
				return foundIndex;
			}
		}

		await(
			MessageDb::instance()->fetchMessagesUntilQueryString(AccountManager::instance()->jid(), m_currentChatJid, foundIndex, searchString),
			this,
			[this](auto result) {
				emit messageSearchFinished(result.queryIndex);
			}
		);
	}

	return -1;
}

int MessageModel::searchForMessageFromOldToNew(const QString &searchString, int startIndex)
{
	int foundIndex = startIndex;

	if (foundIndex >= 0) {
		for (; foundIndex >= 0; foundIndex--) {
			if (m_messages.at(foundIndex).body.contains(searchString, Qt::CaseInsensitive))
				break;
		}
	}

	return foundIndex;
}

void MessageModel::processMessage(Message &msg)
{
	if (msg.body.size() > MESSAGE_MAX_CHARS) {
		auto body = msg.body;
		body.truncate(MESSAGE_MAX_CHARS);
		msg.body = body;
	}
}

void MessageModel::sendPendingMessages()
{
	MessageDb::instance()->fetchPendingMessages(AccountManager::instance()->jid());
}

QXmppMessage::State MessageModel::chatState() const
{
	return m_chatPartnerChatState;
}

void MessageModel::sendChatState(QXmppMessage::State state)
{
	if (!m_rosterItemWatcher.item().chatStateSendingEnabled) {
		return;
	}

	// Handle some special cases
	switch (QXmppMessage::State(state)) {
	case QXmppMessage::State::Composing:
		// Restart timer if new character was typed in
		m_composingTimer->start();
		break;
	case QXmppMessage::State::Active:
		// Start inactive timer when active was sent,
		// so we can set the state to inactive two minutes later
		m_inactiveTimer->start();
		m_composingTimer->stop();
		break;
	default:
		break;
	}

	// Only send if the state changed, filter duplicated
	if (state != m_ownChatState) {
		m_ownChatState = state;
		emit sendChatStateRequested(m_currentChatJid, state);
	}
}

void MessageModel::sendChatState(ChatState::State state)
{
	sendChatState(QXmppMessage::State(state));
}

void MessageModel::correctMessage(const QString &msgId, const QString &message)
{
	const auto hasCorrectId = [&msgId](const Message& msg) {
		return msg.id == msgId;
	};
	auto itr = std::find_if(m_messages.begin(), m_messages.end(), hasCorrectId);

	if (itr != m_messages.end()) {
		Message &msg = *itr;
		msg.body = message;
		if (msg.deliveryState != Enums::DeliveryState::Pending) {
			msg.id = QXmppUtils::generateStanzaUuid();
			// Set replaceId only on first correction, so it's always the original id
			// (`id` is the id of the current edit, `replaceId` is the original id)
			if (!msg.isEdited) {
				msg.isEdited = true;
				msg.replaceId = msgId;
			}
			msg.deliveryState = Enums::DeliveryState::Pending;

			if (ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
				// the trick with the time is important for the servers
				// this way they can tell which version of the message is the latest
				Message copy = msg;
				copy.stamp = QDateTime::currentDateTimeUtc();
				emit sendCorrectedMessageRequested(copy);
			}
		} else if (!msg.isEdited) {
			msg.stamp = QDateTime::currentDateTimeUtc();
		}

		QModelIndex index = createIndex(std::distance(m_messages.begin(), itr), 0);
		emit dataChanged(index, index);

		MessageDb::instance()->updateMessage(msgId, [msg](Message &localMessage) {
			localMessage = msg;
		});
	}
}

void MessageModel::handleChatState(const QString &bareJid, QXmppMessage::State state)
{
	m_chatStateCache[bareJid] = state;

	if (bareJid == m_currentChatJid) {
		m_chatPartnerChatState = state;
		m_chatPartnerChatStateTimeout->start();
		emit chatStateChanged();
	}
}

void MessageModel::showMessageNotification(const Message &message, MessageOrigin origin) const
{
	// Send a notification in the following cases:
	// * The message was not sent by the user from another resource and
	//   received via Message Carbons.
	// * Notifications from the chat partner are not muted.
	// * The corresponding chat is not opened while the application window
	//   is active.

	switch (origin) {
	case MessageOrigin::UserInput:
	case MessageOrigin::MamInitial:
	case MessageOrigin::MamBacklog:
		// no notifications
		return;
	case MessageOrigin::Stream:
	case MessageOrigin::MamCatchUp:
		break;
	}

	if (!message.isOwn) {
		const auto accountJid = AccountManager::instance()->jid();
		const auto chatJid = message.from;

		bool userMuted = Kaidan::instance()->notificationsMuted(chatJid);
		bool chatActive =
				isChatCurrentChat(accountJid, chatJid) &&
				QGuiApplication::applicationState() == Qt::ApplicationActive;

		if (!userMuted && !chatActive) {
			Notifications::instance()->sendMessageNotification(accountJid, chatJid, message.id, message.body);
		}
	}
}

bool MessageModel::mamLoading() const
{
	return m_mamLoading;
}

void MessageModel::setMamLoading(bool mamLoading)
{
	if (m_mamLoading != mamLoading) {
		m_mamLoading = mamLoading;
		emit mamLoadingChanged();
	}
}

void MessageModel::handleKeysRetrieved(const QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> &keys)
{
	m_keys = keys;
	emit keysChanged();

	// The messages need to be updated in order to reflect the most recent trust
	// levels.
	if (!m_messages.isEmpty()) {
		emit dataChanged(index(0), index(m_messages.size() - 1), { IsTrusted });
	}
}

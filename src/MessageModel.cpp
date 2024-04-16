// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Tibor Csötönyi <dev@taibsu.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

bool DisplayedMessageReaction::operator<(const DisplayedMessageReaction &other) const
{
	return emoji < other.emoji;
}

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

	connect(&m_accountOmemoWatcher, &OmemoWatcher::usableDevicesChanged, this, &MessageModel::usableOmemoDevicesChanged);
	connect(&m_contactOmemoWatcher, &OmemoWatcher::usableDevicesChanged, this, &MessageModel::usableOmemoDevicesChanged);

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
		Q_EMIT chatStateChanged();
	});

	// addMessage requests are forwarded to the MessageDb, are deduplicated there and
	// added if MessageDb::messageAdded is emitted
	connect(MessageDb::instance(), &MessageDb::messageAdded, this, &MessageModel::handleMessage);

	connect(MessageDb::instance(), &MessageDb::messageUpdated, this, &MessageModel::handleMessageUpdated);

	connect(this, &MessageModel::handleChatStateRequested,
	        this, &MessageModel::handleChatState);

	connect(MessageDb::instance(), &MessageDb::allMessagesRemovedFromChat, this, &MessageModel::removeMessages);

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
	roles[SenderId] = "senderId";
	roles[Id] = "id";
	roles[IsLastRead] = "isLastRead";
	roles[IsEdited] = "isEdited";
	roles[Date] = "date";
	roles[NextDate] = "nextDate";
	roles[Time] = "time";
	roles[Body] = "body";
	roles[Encryption] = "encryption";
	roles[IsTrusted] = "isTrusted";
	roles[DeliveryState] = "deliveryState";
	roles[DeliveryStateIcon] = "deliveryStateIcon";
	roles[DeliveryStateName] = "deliveryStateName";
	roles[IsSpoiler] = "isSpoiler";
	roles[SpoilerHint] = "spoilerHint";
	roles[IsOwn] = "isOwn";
	roles[Files] = "files";
	roles[DisplayedReactions] = "displayedReactions";
	roles[DetailedReactions] = "detailedReactions";
	roles[OwnDetailedReactions] = "ownDetailedReactions";
	roles[ErrorText] = "errorText";
	return roles;
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
	const auto row = index.row();

	if (!hasIndex(row, index.column(), index.parent())) {
		qWarning() << "Could not get data from message model." << index << role;
		return {};
	}
	const Message &msg = m_messages.at(row);

	switch (role) {
	case SenderId:
		return msg.senderId;
	case Id:
		return msg.id;
	case IsLastRead:
		// A read marker text is only displayed if the message is the last read message and no
		// message is received by the contact after it.
		if (msg.id == m_lastReadOwnMessageId) {
			for (auto i = index.row(); i >= 0; --i) {
				if (m_messages.at(i).senderId != m_currentAccountJid) {
					return false;
				}
			}
			return true;
		}
		return false;
	case IsEdited:
		return !msg.replaceId.isEmpty();
	case Date:
		return formatDate(msg.timestamp.toLocalTime().date());
	case NextDate:
		return formatDate(searchNextDate(row));
	case Time:
		return QLocale::system().toString(msg.timestamp.toLocalTime().time(), QLocale::ShortFormat);
	case Body:
		return msg.body;
	case Encryption:
		return msg.encryption;
	case IsTrusted: {
		if (msg.isOwn() && msg.senderKey.isEmpty()) {
			return true;
		}

		const auto trustLevel = m_keys.value(msg.isOwn() ? msg.accountJid : msg.senderId).value(msg.senderKey);
		return (QXmpp::TrustLevel::AutomaticallyTrusted | QXmpp::TrustLevel::ManuallyTrusted | QXmpp::TrustLevel::Authenticated).testFlag(trustLevel);
	}
	case DeliveryState:
		return QVariant::fromValue(msg.deliveryState);
	case DeliveryStateIcon:
		switch (msg.deliveryState) {
		case DeliveryState::Pending:
			return QmlUtils::getResourcePath(QStringLiteral("images/dots.svg"));
		case DeliveryState::Sent:
			return QmlUtils::getResourcePath(QStringLiteral("images/check-mark-pale.svg"));
		case DeliveryState::Delivered:
			return QmlUtils::getResourcePath(QStringLiteral("images/check-mark.svg"));
		case DeliveryState::Error:
			return QmlUtils::getResourcePath(QStringLiteral("images/cross.svg"));
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
	case IsSpoiler:
		return msg.isSpoiler;
	case SpoilerHint:
		return msg.spoilerHint;
	case IsOwn:
		return msg.isOwn();
	case Files:
		return QVariant::fromValue(msg.files);
	case DisplayedReactions: {
		QVector<DisplayedMessageReaction> displayedMessageReactions;

		const auto &reactionSenders = msg.reactionSenders;
		for (auto itr = reactionSenders.begin(); itr != reactionSenders.end(); ++itr) {
			const auto ownReactionsIterated = itr.key() == m_currentAccountJid;

			for (const auto &reaction : std::as_const(itr->reactions)) {
				auto reactionItr = std::find_if(displayedMessageReactions.begin(), displayedMessageReactions.end(), [=](const DisplayedMessageReaction &displayedMessageReaction) {
					return displayedMessageReaction.emoji == reaction.emoji;
				});

				if (ownReactionsIterated) {
					if (reactionItr == displayedMessageReactions.end()) {
						displayedMessageReactions.append({ reaction.emoji, 1, ownReactionsIterated, reaction.deliveryState });
					} else {
						reactionItr->count++;
						reactionItr->ownReactionIncluded = ownReactionsIterated;
						reactionItr->deliveryState = reaction.deliveryState;
					}
				} else {
					if (reactionItr == displayedMessageReactions.end()) {
						displayedMessageReactions.append({ reaction.emoji, 1, ownReactionsIterated, {} });
					} else {
						reactionItr->count++;
					}
				}
			}
		}

		std::sort(displayedMessageReactions.begin(), displayedMessageReactions.end());

		return QVariant::fromValue(displayedMessageReactions);
	}
	case DetailedReactions: {
		QVector<DetailedMessageReaction> detailedMessageReactions;

		const auto &reactionSenders = msg.reactionSenders;
		for (auto itr = reactionSenders.begin(); itr != reactionSenders.end(); ++itr) {
			// Skip own reactions.
			if (itr.key() != m_currentAccountJid) {
				QStringList emojis;

				for (const auto &reaction : std::as_const(itr->reactions)) {
					emojis.append(reaction.emoji);
				}

				std::sort(emojis.begin(), emojis.end());

				detailedMessageReactions.append({ itr.key(), emojis });
			}
		}

		return QVariant::fromValue(detailedMessageReactions);
	}
	case OwnDetailedReactions:
		return QVariant::fromValue(msg.reactionSenders.value(m_currentAccountJid).reactions);
	case ErrorText:
		return msg.errorText;
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
					await(MessageDb::instance()->fetchMessagesUntilFirstContactMessage(AccountManager::instance()->jid(), m_currentChatJid, 0), this, [this](QVector<Message> &&messages) {
						handleMessagesFetched(messages);
					});
				} else {
					await(MessageDb::instance()->fetchMessagesUntilId(AccountManager::instance()->jid(), m_currentChatJid, 0, lastReadContactMessageId), this, [this](QVector<Message> &&messages) {
						handleMessagesFetched(messages);
					});
				}
			} else {
				await(MessageDb::instance()->fetchMessages(AccountManager::instance()->jid(), m_currentChatJid, 0), this, [this](QVector<Message> &&messages) {
					handleMessagesFetched(messages);
				});
			}
		} else {
			await(MessageDb::instance()->fetchMessages(AccountManager::instance()->jid(), m_currentChatJid, m_messages.size()), this, [this](QVector<Message> &&messages) {
				handleMessagesFetched(messages);
			});
		}
	} else if (!m_fetchedAllFromMam) {
		// use earliest timestamp
		const auto lastStamp = [this]() -> QDateTime {
			const auto stamp1 = m_mamBacklogLastStamp.isNull() ? QDateTime::currentDateTimeUtc() : m_mamBacklogLastStamp;
			if (!m_messages.empty()) {
				return std::min(stamp1, m_messages.constLast().timestamp);
			}
			return stamp1;
		};

		// Skip unneeded steps when 'canFetchMore()' has not been called before calling
		// 'fetchMore()'.
		if (!m_mamLoading) {
			Q_EMIT Kaidan::instance()->client()->messageHandler()->retrieveBacklogMessagesRequested(m_currentChatJid, lastStamp());
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

bool MessageModel::isChatCurrentChat(const QString &accountJid, const QString &chatJid) const
{
	return accountJid == m_currentAccountJid && chatJid == m_currentChatJid;
}

const RosterItemWatcher &MessageModel::rosterItemWatcher() const
{
	return m_rosterItemWatcher;
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
	Q_EMIT encryptionChanged();
}

QList<OmemoManager::Device> MessageModel::usableOmemoDevices() const
{
	   return m_currentAccountJid == m_currentChatJid ? m_accountOmemoWatcher.usableDevices() : m_contactOmemoWatcher.usableDevices();
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
	const auto readMessage = m_messages.at(readMessageIndex);

	// Search for the last read contact message if it is at the top of the chat page list view but
	// readMessageIndex is of an own message.
	if (readMessage.isOwn()) {
		auto isContactMessageRead = false;

		for (int i = readMessageIndex + 1; i != m_messages.size(); ++i) {
			if (const auto &message = m_messages.at(i); !message.isOwn()) {
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

		Q_EMIT RosterModel::instance()->updateItemRequested(m_currentChatJid, [=, this](RosterItem &item) {
			item.lastReadContactMessageId = readMessageId;
			item.readMarkerPending = readMarkerPending;

			// If the read message is the latest one, lastReadContactMessageId is empty or the read
			// message is an own message, reset the counter for unread messages.
			// lastReadContactMessageId can be empty if there is no contact message stored or the
			// oldest stored contact message is marked as first unread.
			// Otherwise, decrease it by the number of contact messages between the read contact
			// message and the last read contact message.
			if (readContactMessageIndex == 0 ||
				lastReadContactMessageId.isEmpty() ||
				readMessage.isOwn())
			{
				item.unreadMessages = 0;
			} else {
				int readMessageCount = 1;
				for (int i = readContactMessageIndex + 1; i < m_messages.size(); ++i) {
					if (const auto &message = m_messages.at(i); message.id == lastReadContactMessageId) {
						break;
					} else if (!message.isOwn()) {
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
		if (!message.isOwn() && (messageId == lastReadContactMessageId || lastReadContactMessageId.isEmpty())) {
			lastReadContactMessageIndex = i;
		}
	}

	if (lastReadContactMessageIndex > 0) {
		for (auto i = lastReadContactMessageIndex - 1; i >= 0; --i) {
			if (!m_messages.at(i).isOwn()) {
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
		if (!m_messages.at(i).isOwn()) {
			++unreadMessageCount;
		}
	}

	// Search for the first contact message after the marked one.
	if (index < m_messages.size() - 1) {
		for (int i = index + 1; i != m_messages.size(); ++i) {
			const auto &message = m_messages.at(i);
			if (!message.isOwn()) {
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
			Q_EMIT RosterModel::instance()->updateItemRequested(currentChatJid, [=](RosterItem &item) {
				item.unreadMessages = unreadMessageCount;
				item.lastReadContactMessageId = firstContactMessageId;
			});
		});
	} else  {
		Q_EMIT RosterModel::instance()->updateItemRequested(m_currentChatJid, [=](RosterItem &item) {
			item.unreadMessages = unreadMessageCount;
			item.lastReadContactMessageId = lastReadContactMessageId;
		});
	}
}

void MessageModel::addMessageReaction(const QString &messageId, const QString &emoji)
{
	const auto itr = std::find_if(m_messages.cbegin(), m_messages.cend(), [&](const Message &message) {
		return message.id == messageId;
	});

	if (itr != m_messages.cend()) {
		const auto senderJid = m_currentAccountJid;
		const auto reactions = itr->reactionSenders.value(senderJid).reactions;

		if (undoMessageReactionRemoval(messageId, senderJid, emoji, reactions)) {
			return;
		}

		auto addReaction = [messageId, senderJid, emoji](MessageReactionDeliveryState::Enum deliveryState) -> QFuture<void> {
			return MessageDb::instance()->updateMessage(messageId, [senderJid, emoji, deliveryState](Message &message) {
				auto &reactionSender = message.reactionSenders[senderJid];
				reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();

				MessageReaction reaction;
				reaction.emoji = emoji;
				reaction.deliveryState = deliveryState;

				auto &reactions = reactionSender.reactions;
				reactions.append(reaction);
			});
		};

		auto future = addReaction(MessageReactionDeliveryState::PendingAddition);
		await(future, this, [=, this, chatJid = m_currentChatJid]() {
			if (ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
				QVector<QString> emojis;

				for (const auto &reaction : reactions) {
					switch(reaction.deliveryState) {
					case MessageReactionDeliveryState::PendingAddition:
					case MessageReactionDeliveryState::ErrorOnAddition:
					case MessageReactionDeliveryState::Sent:
					case MessageReactionDeliveryState::Delivered:
						emojis.append(reaction.emoji);
						break;
					default:
						break;
					}
				}

				emojis.append(emoji);

				runOnThread(Kaidan::instance()->client()->messageHandler(), [chatJid, messageId, emojis] {
					return Kaidan::instance()->client()->messageHandler()->sendMessageReaction(chatJid, messageId, emojis);
				}, this, [=, this](QFuture<QXmpp::SendResult> future) {
					await(future, this, [=, this](QXmpp::SendResult result) {
						if (const auto error = std::get_if<QXmppError>(&result)) {
							Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Reaction could not be sent: %1").arg(error->description));
							addReaction(MessageReactionDeliveryState::ErrorOnAddition);
						} else {
							updateMessageReactionsAfterSending(messageId, senderJid);
						}
					});
				});
			}
		});
	}
}

void MessageModel::removeMessageReaction(const QString &messageId, const QString &emoji)
{
	const auto itr = std::find_if(m_messages.cbegin(), m_messages.cend(), [&](const Message &message) {
		return message.id == messageId;
	});

	if (itr != m_messages.cend()) {
		const auto senderJid = m_currentAccountJid;
		const auto &reactions = itr->reactionSenders.value(senderJid).reactions;

		if (undoMessageReactionAddition(messageId, senderJid, emoji, reactions)) {
			return;
		}

		auto future = MessageDb::instance()->updateMessage(messageId, [senderJid, emoji](Message &message) {
			auto &reactionSenders = message.reactionSenders;
			auto &reactions = reactionSenders[senderJid].reactions;

			const auto itr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
				return reaction.emoji == emoji;
			});

			switch (auto &deliveryState = itr->deliveryState) {
			case MessageReactionDeliveryState::Sent:
				deliveryState = MessageReactionDeliveryState::PendingRemovalAfterSent;
				break;
			case MessageReactionDeliveryState::Delivered:
				deliveryState = MessageReactionDeliveryState::PendingRemovalAfterDelivered;
				break;
			default:
				break;
			}
		});

		await(future, this, [=, this, chatJid = m_currentChatJid]() {
			if (ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
				auto &reactionSenders = itr->reactionSenders;
				auto &reactions = reactionSenders[senderJid].reactions;
				QVector<QString> emojis;

				for (auto &reaction : reactions) {
					const auto &storedEmoji = reaction.emoji;
					const auto deliveryState = reaction.deliveryState;

					switch (deliveryState) {
					case MessageReactionDeliveryState::PendingAddition:
					case MessageReactionDeliveryState::ErrorOnAddition:
					case MessageReactionDeliveryState::Sent:
					case MessageReactionDeliveryState::Delivered:
						emojis.append(storedEmoji);
						break;
					default:
						break;
					}
				}

				runOnThread(Kaidan::instance()->client()->messageHandler(), [chatJid, messageId, emojis] {
					return Kaidan::instance()->client()->messageHandler()->sendMessageReaction(chatJid, messageId, emojis);
				}, this, [=, this](QFuture<QXmpp::SendResult> future) {
					await(future, this, [=, this](QXmpp::SendResult result) {
						if (const auto error = std::get_if<QXmppError>(&result)) {
							Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Reaction could not be sent: %1").arg(error->description));

							MessageDb::instance()->updateMessage(messageId, [senderJid, emoji](Message &message) {
								auto &reactions = message.reactionSenders[senderJid].reactions;

								const auto itr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
									return reaction.emoji == emoji;
								});

								switch (auto &deliveryState = itr->deliveryState) {
								case MessageReactionDeliveryState::Sent:
									deliveryState = MessageReactionDeliveryState::ErrorOnRemovalAfterSent;
									break;
								case MessageReactionDeliveryState::Delivered:
									deliveryState = MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered;
									break;
								default:
									break;
								}
							});
						} else {
							updateMessageReactionsAfterSending(messageId, senderJid);
						}
					});
				});
			}
		});
	}
}

void MessageModel::resendMessageReactions(const QString &messageId)
{
	const auto itr = std::find_if(m_messages.cbegin(), m_messages.cend(), [&](const Message &message) {
		return message.id == messageId;
	});

	if (itr != m_messages.cend()) {
		const auto senderJid = m_currentAccountJid;

		MessageDb::instance()->updateMessage(messageId, [senderJid](Message &message) {
			auto &reactionSender = message.reactionSenders[senderJid];
			reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();

			for (auto &reaction : reactionSender.reactions) {
				switch (auto &deliveryState = reaction.deliveryState) {
				case MessageReactionDeliveryState::ErrorOnAddition:
					deliveryState = MessageReactionDeliveryState::PendingAddition;
					break;
				case MessageReactionDeliveryState::ErrorOnRemovalAfterSent:
					deliveryState = MessageReactionDeliveryState::PendingRemovalAfterSent;
					break;
				case MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered:
					deliveryState = MessageReactionDeliveryState::PendingRemovalAfterDelivered;
					break;
				default:
					break;
				}
			}
		});

		if (ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
			QVector<QString> emojis;

			for (const auto &reaction : itr->reactionSenders.value(m_currentAccountJid).reactions) {
				if (const auto deliveryState = reaction.deliveryState;
					deliveryState != MessageReactionDeliveryState::PendingRemovalAfterSent &&
					deliveryState != MessageReactionDeliveryState::PendingRemovalAfterDelivered &&
					deliveryState != MessageReactionDeliveryState::ErrorOnRemovalAfterSent &&
					deliveryState != MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered) {
					emojis.append(reaction.emoji);
				}
			}

			runOnThread(Kaidan::instance()->client()->messageHandler(), [chatJid = m_currentChatJid, messageId, emojis] {
				return Kaidan::instance()->client()->messageHandler()->sendMessageReaction(chatJid, messageId, emojis);
			}, this, [=, this](QFuture<QXmpp::SendResult> future) {
				await(future, this, [=, this](QXmpp::SendResult result) {
					if (const auto error = std::get_if<QXmppError>(&result)) {
						Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Reactions could not be sent: %1").arg(error->description));

						MessageDb::instance()->updateMessage(messageId, [senderJid](Message &message) {
							auto &reactionSender = message.reactionSenders[senderJid];
							reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();

							for (auto &reaction : reactionSender.reactions) {
								switch (auto &deliveryState = reaction.deliveryState) {
								case MessageReactionDeliveryState::PendingAddition:
									deliveryState = MessageReactionDeliveryState::ErrorOnAddition;
									break;
								case MessageReactionDeliveryState::PendingRemovalAfterSent:
									deliveryState = MessageReactionDeliveryState::ErrorOnRemovalAfterSent;
									break;
								case MessageReactionDeliveryState::PendingRemovalAfterDelivered:
									deliveryState = MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered;
									break;
								default:
									break;
								}
							}
						});
					} else {
						updateMessageReactionsAfterSending(messageId, senderJid);
					}
				});
			});
		}
	}
}

void MessageModel::sendPendingMessageReactions(const QString &accountJid)
{
	auto future = MessageDb::instance()->fetchPendingReactions(accountJid);
	await(future, this, [=, this](QMap<QString, QMap<QString, MessageReactionSender>> reactions) {
		for (auto reactionItr = reactions.cbegin(); reactionItr != reactions.cend(); ++reactionItr) {
			const auto &reactionSenders = reactionItr.value();

			for (auto reactionSenderItr = reactionSenders.cbegin(); reactionSenderItr != reactionSenders.cend(); ++reactionSenderItr) {
				const auto messageId = reactionSenderItr.key();
				QVector<QString> emojis;

				for (const auto &reaction : reactionSenderItr->reactions) {
					if (const auto deliveryState = reaction.deliveryState;
						deliveryState != MessageReactionDeliveryState::PendingRemovalAfterSent &&
						deliveryState != MessageReactionDeliveryState::PendingRemovalAfterDelivered &&
						deliveryState != MessageReactionDeliveryState::ErrorOnRemovalAfterSent &&
						deliveryState != MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered) {
						emojis.append(reaction.emoji);
					}
				}

				runOnThread(Kaidan::instance()->client()->messageHandler(), [chatJid = reactionItr.key(), messageId, emojis] {
					return Kaidan::instance()->client()->messageHandler()->sendMessageReaction(chatJid, messageId, emojis);
				}, this, [=, this](QFuture<QXmpp::SendResult> future) {
					await(future, this, [=, this](QXmpp::SendResult result) {
						if (const auto error = std::get_if<QXmppError>(&result)) {
							Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Reaction could not be sent: %1").arg(error->description));

							MessageDb::instance()->updateMessage(messageId, [accountJid](Message &message) {
								auto &reactionSender = message.reactionSenders[accountJid];
								reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();

								for (auto &reaction : reactionSender.reactions) {
									switch (auto &deliveryState = reaction.deliveryState) {
									case MessageReactionDeliveryState::PendingAddition:
										deliveryState = MessageReactionDeliveryState::ErrorOnAddition;
										break;
									case MessageReactionDeliveryState::PendingRemovalAfterSent:
										deliveryState = MessageReactionDeliveryState::ErrorOnRemovalAfterSent;
										break;
									case MessageReactionDeliveryState::PendingRemovalAfterDelivered:
										deliveryState = MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered;
										break;
									default:
										break;
									}
								}
							});
						} else {
							updateMessageReactionsAfterSending(messageId, accountJid);
						}
					});
				});
			}
		}


	});
}

bool MessageModel::canCorrectMessage(int index) const
{
	// check index validity
	if (index < 0 || index >= m_messages.size())
		return false;

	// message needs to be sent by us and needs to be no error message
	const auto &msg = m_messages.at(index);
	if (!msg.isOwn() || msg.deliveryState == Enums::DeliveryState::Error)
		return false;

	// check time limit
	const auto timeThreshold =
		QDateTime::currentDateTimeUtc().addDays(-MAX_CORRECTION_MESSAGE_DAYS_DEPTH);
	if (msg.timestamp < timeThreshold)
		return false;

	// check messages count limit
	for (int i = 0, count = 0; i < index; i++) {
		if (m_messages.at(i).isOwn() && ++count == MAX_CORRECTION_MESSAGE_COUNT_DEPTH)
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
			Q_EMIT messageFetchingFinished();
		}

		return;
	}

	beginInsertRows(QModelIndex(), rowCount(), rowCount() + msgs.length() - 1);
	for (auto msg : msgs) {
		// Skip messages that were not fetched for the current chat.
		if (msg.accountJid != m_currentAccountJid || msg.chatJid != m_currentChatJid) {
			continue;
		}

		processMessage(msg);
		m_messages << msg;
	}
	endInsertRows();

	Q_EMIT messageFetchingFinished();
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
				if (!message.isOwn()) {
					Q_EMIT RosterModel::instance()->updateItemRequested(m_currentChatJid, [=, messageId = message.id](RosterItem &item) {
						item.lastReadContactMessageId = messageId;
					});
					break;
				}
			}
		}

		Q_EMIT messageFetchingFinished();
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
	Q_EMIT currentAccountJidChanged(accountJid);
	Q_EMIT currentChatJidChanged(chatJid);

	m_rosterItemWatcher.setJid(chatJid);
	m_contactResourcesWatcher.setJid(chatJid);
	m_accountOmemoWatcher.setJid(accountJid);
	m_contactOmemoWatcher.setJid(chatJid);
	m_lastReadOwnMessageId = m_rosterItemWatcher.item().lastReadOwnMessageId;

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
	if (accountJid == m_currentAccountJid && chatJid == m_currentChatJid) {
		removeAllMessages();
	}
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
		if (msg.timestamp > message.timestamp) {
			insertMessage(i, msg);
			return;
		}
		i++;
	}
	// add message to the end of the list
	insertMessage(i, msg);
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
			Q_EMIT dataChanged(modelIndex, modelIndex, { IsLastRead });

			if (lastReadOwnMessageIndex != -1) {
				break;
			}
		} else if (message.id == m_lastReadOwnMessageId) {
			lastReadOwnMessageIndex = i;

			const auto modelIndex = index(lastReadOwnMessageIndex);
			Q_EMIT dataChanged(modelIndex, modelIndex, { IsLastRead });

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

	if (msg.accountJid == m_currentAccountJid && msg.chatJid == m_currentChatJid) {
		addMessage(std::move(msg));
	}
}

void MessageModel::handleMessageUpdated(const Message &message)
{
	for (int i = 0; i < m_messages.length(); i++) {
		const auto oldMessage = m_messages.at(i);
		const auto oldId = oldMessage.id;
		const auto oldReplaceId = oldMessage.replaceId;

		// The updated message can be either a normal message, a first message correction or a
		// subsequent messsage correction.
		if ((!oldId.isEmpty() && oldId == message.id) ||
			(!oldId.isEmpty() && oldId == message.replaceId) ||
			(!oldReplaceId.isEmpty() && oldReplaceId == message.replaceId)) {
			const auto oldMessageTimestamp = m_messages.at(i).timestamp;

			beginRemoveRows(QModelIndex(), i, i);
			m_messages.removeAt(i);
			endRemoveRows();

			// Insert the message at its original position if the date is unchanged.
			// Otherwise, move it to its new position.
			if (!m_messages.isEmpty() && (message.timestamp == oldMessageTimestamp)) {
				insertMessage(i, message);
			} else {
				addMessage(message);
			}

			showMessageNotification(message, MessageOrigin::Stream);

			break;
		}
	}
}

int MessageModel::searchForMessageFromNewToOld(const QString &searchString, int startIndex)
{
	int foundIndex = startIndex;

	if (foundIndex < m_messages.size()) {
		for (; foundIndex < m_messages.size(); foundIndex++) {
			if (foundIndex > -1 && m_messages.at(foundIndex).body.contains(searchString, Qt::CaseInsensitive)) {
				return foundIndex;
			}
		}

		await(
			MessageDb::instance()->fetchMessagesUntilQueryString(AccountManager::instance()->jid(), m_currentChatJid, foundIndex, searchString),
			this,
			[this](MessageDb::MessageResult &&result) {
				handleMessagesFetched(result.messages);
				Q_EMIT messageSearchFinished(result.queryIndex);
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

QXmppMessage::State MessageModel::chatState() const
{
	return m_chatPartnerChatState;
}

void MessageModel::sendChatState(QXmppMessage::State state)
{
	if (m_contactResourcesWatcher.resourcesCount() == 0 || !m_rosterItemWatcher.item().chatStateSendingEnabled) {
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
		Q_EMIT sendChatStateRequested(m_currentChatJid, state);
	}
}

void MessageModel::sendChatState(ChatState::State state)
{
	sendChatState(QXmppMessage::State(state));
}

void MessageModel::correctMessage(const QString &replaceId, const QString &body, const QString &spoilerHint)
{
	const auto hasCorrectId = [&replaceId](const Message &message) {
		return message.id == replaceId;
	};
	auto itr = std::find_if(m_messages.begin(), m_messages.end(), hasCorrectId);

	if (itr != m_messages.end()) {
		Message &message = *itr;
		message.body = body;
		message.isSpoiler = !spoilerHint.isEmpty();
		message.spoilerHint = spoilerHint;

		if (message.deliveryState != Enums::DeliveryState::Pending) {
			message.id = QXmppUtils::generateStanzaUuid();
			// Set replaceId only on first correction, so it's always the original id
			// (`id` is the id of the current edit, `replaceId` is the original id)
			if (message.replaceId.isEmpty()) {
				message.replaceId = replaceId;
			}
			message.deliveryState = Enums::DeliveryState::Pending;

			if (ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
				// the trick with the time is important for the servers
				// this way they can tell which version of the message is the latest
				Message copy = message;
				copy.timestamp = QDateTime::currentDateTimeUtc();
				Q_EMIT sendCorrectedMessageRequested(copy);
			}
		} else if (message.replaceId.isEmpty()) {
			message.timestamp = QDateTime::currentDateTimeUtc();
		}

		QModelIndex index = createIndex(std::distance(m_messages.begin(), itr), 0);
		Q_EMIT dataChanged(index, index);

		MessageDb::instance()->updateMessage(replaceId, [message](Message &localMessage) {
			localMessage = message;
		});
	}
}

void MessageModel::removeMessage(const QString &messageId)
{
	const auto hasCorrectId = [&messageId](const Message &message) {
		return message.id == messageId;
	};

	const Message *const itr = std::find_if(m_messages.begin(), m_messages.end(), hasCorrectId);

	// Update the roster item of the current chat.
	if (itr != m_messages.cend()) {
		int readMessageIndex = std::distance(m_messages.cbegin(), itr);

		const QString &lastReadContactMessageId = m_rosterItemWatcher.item().lastReadContactMessageId;
		const QString &lastReadOwnMessageId = m_rosterItemWatcher.item().lastReadOwnMessageId;

		if (lastReadContactMessageId == messageId || lastReadOwnMessageId == messageId) {
			handleMessageRead(readMessageIndex);

			// Get the previous message ID if possible.
			const int newLastReadMessageIndex = readMessageIndex + 1;
			const bool isNewLastReadMessageIdValid = m_messages.length() >= newLastReadMessageIndex;

			const QString newLastReadMessageId {
				isNewLastReadMessageIdValid && newLastReadMessageIndex < m_messages.length() ?
					m_messages.at(newLastReadMessageIndex).id :
					QString()
			};

			if (newLastReadMessageId.isEmpty()) {
				RosterModel::instance()->updateItem(m_currentChatJid, [=](RosterItem &item) {
					item.lastReadContactMessageId = QString();
					item.lastReadOwnMessageId = QString();
					item.lastMessage = QString();
					item.lastMessageSenderId = QString();
					item.unreadMessages = 0;
				});
			} else {
				Q_EMIT RosterModel::instance()->updateItemRequested(m_currentChatJid,
					[=](RosterItem &item) {
						if (itr->isOwn()) {
							item.lastReadOwnMessageId = newLastReadMessageId;
						} else {
							item.lastReadContactMessageId = newLastReadMessageId;
						}
					});
			}
		}

		// Remove the message from the database and model.

		MessageDb::instance()->removeMessage(itr->accountJid, itr->chatJid, messageId);
		updateLastReadOwnMessageId();

		QModelIndex index = createIndex(readMessageIndex, 0);

		beginRemoveRows(QModelIndex(), readMessageIndex, readMessageIndex);
		m_messages.removeAt(readMessageIndex);
		endRemoveRows();

		Q_EMIT dataChanged(index, index);
	}
}

void MessageModel::handleChatState(const QString &bareJid, QXmppMessage::State state)
{
	m_chatStateCache[bareJid] = state;

	if (bareJid == m_currentChatJid) {
		m_chatPartnerChatState = state;
		m_chatPartnerChatStateTimeout->start();
		Q_EMIT chatStateChanged();
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

	if (!message.isOwn()) {
		const auto accountJid = AccountManager::instance()->jid();
		const auto chatJid = message.chatJid;

		bool userMuted = m_rosterItemWatcher.item().notificationsMuted;
		bool chatActive =
				isChatCurrentChat(accountJid, chatJid) &&
				QGuiApplication::applicationState() == Qt::ApplicationActive;

		if (!userMuted && !chatActive) {
			Notifications::instance()->sendMessageNotification(accountJid, chatJid, message.id, message.body);
		}
	}
}

bool MessageModel::undoMessageReactionRemoval(const QString &messageId, const QString &senderJid, const QString &emoji, const QVector<MessageReaction> &reactions)
{
	const auto reactionItr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
		return reaction.emoji == emoji;
	});

	if (reactionItr != reactions.end()) {
		MessageDb::instance()->updateMessage(messageId, [senderJid, emoji](Message &message) {
			auto &reactions = message.reactionSenders[senderJid].reactions;

			const auto itr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
				return reaction.emoji == emoji;
			});

			switch (auto &deliveryState = itr->deliveryState) {
			case MessageReactionDeliveryState::PendingRemovalAfterSent:
			case MessageReactionDeliveryState::ErrorOnRemovalAfterSent:
				deliveryState = MessageReactionDeliveryState::Sent;
				break;
			case MessageReactionDeliveryState::PendingRemovalAfterDelivered:
			case MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered:
				deliveryState = MessageReactionDeliveryState::Delivered;
				break;
			default:
				break;
			}
		});

		return true;
	}

	return false;
}

bool MessageModel::undoMessageReactionAddition(const QString &messageId, const QString &senderJid, const QString &emoji, const QVector<MessageReaction> &reactions)
{
	const auto reactionItr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
		return reaction.emoji == emoji &&
			(reaction.deliveryState == MessageReactionDeliveryState::PendingAddition ||
			 reaction.deliveryState == MessageReactionDeliveryState::ErrorOnAddition);
	});

	if (reactionItr != reactions.end()) {
		MessageDb::instance()->updateMessage(messageId, [senderJid, emoji](Message &message) {
			auto &reactionSenders = message.reactionSenders;
			auto &reactions = reactionSenders[senderJid].reactions;

			const auto itr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
				return reaction.emoji == emoji;
			});

			switch (itr->deliveryState) {
			case MessageReactionDeliveryState::PendingAddition:
			case MessageReactionDeliveryState::ErrorOnAddition:
				reactions.erase(itr);

				// Remove the reaction sender if it has no reactions anymore.
				if (reactions.isEmpty()) {
					reactionSenders.remove(senderJid);
				}

				break;
			default:
				break;
			}
		});

		return true;
	}

	return false;
}

void MessageModel::updateMessageReactionsAfterSending(const QString &messageId, const QString &senderJid)
{
	MessageDb::instance()->updateMessage(messageId, [senderJid](Message &message) {
		auto &reactionSenders = message.reactionSenders;
		auto &reactionSender = message.reactionSenders[senderJid];
		reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();
		auto &reactions = reactionSender.reactions;

		for (auto itr = reactions.begin(); itr != reactions.end();) {
			switch (auto &deliveryState = itr->deliveryState) {
			case MessageReactionDeliveryState::PendingAddition:
			case MessageReactionDeliveryState::ErrorOnAddition:
				deliveryState = MessageReactionDeliveryState::Sent;
				++itr;
				break;
			case MessageReactionDeliveryState::PendingRemovalAfterSent:
			case MessageReactionDeliveryState::PendingRemovalAfterDelivered:
			case MessageReactionDeliveryState::ErrorOnRemovalAfterSent:
			case MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered:
				itr = reactions.erase(itr);
				break;
			default:
				++itr;
				break;
			}
		}

		// Remove the reaction sender if it has no reactions anymore.
		if (reactions.isEmpty()) {
			reactionSenders.remove(senderJid);
		}
	});
}

bool MessageModel::mamLoading() const
{
	return m_mamLoading;
}

void MessageModel::setMamLoading(bool mamLoading)
{
	if (m_mamLoading != mamLoading) {
		m_mamLoading = mamLoading;
		Q_EMIT mamLoadingChanged();
	}
}

void MessageModel::handleKeysRetrieved(const QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> &keys)
{
	m_keys = keys;
	Q_EMIT keysChanged();

	// The messages need to be updated in order to reflect the most recent trust levels.
	if (!m_messages.isEmpty()) {
		Q_EMIT dataChanged(index(0), index(m_messages.size() - 1), { IsTrusted });
	}
}

QDate MessageModel::searchNextDate(int messageStartIndex) const
{
	const auto startDate = m_messages.at(messageStartIndex).timestamp.toLocalTime().date();

	for (int i = messageStartIndex; i >= 0; i--) {
		if (const auto date = m_messages.at(i).timestamp.toLocalTime().date(); date > startDate) {
			return date;
		}
	}

	return {};
}

QString MessageModel::formatDate(QDate localDate) const
{
	if (localDate.isNull()) {
		// Unset date: Return a default-constructed string.
		return {};
	} else if (const auto elapsedNightCount = localDate.daysTo(QDateTime::currentDateTime().date()); elapsedNightCount == 0) {
		// Today: Return that term.
		return tr("Today");
	} else if (elapsedNightCount == 1) {
		// Yesterday: Return that term.
		return tr("Yesterday");
	} else if (elapsedNightCount <= 7) {
		// Between yesterday and seven days before today: Return the day of the week.
		return QLocale::system().dayName(localDate.dayOfWeek(), QLocale::LongFormat);
	} else {
		// Older than seven days before today: Return the date.
		return QLocale::system().toString(localDate, QLocale::LongFormat);
	}
}

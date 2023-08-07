// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Tibor Csötönyi <dev@taibsu.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>
// QXmpp
#include <QXmppMessage.h>
// Kaidan
#include "Message.h"
#include "OmemoWatcher.h"
#include "PresenceCache.h"
#include "RosterItemWatcher.h"

class QTimer;
class Kaidan;

class ChatState : public QObject
{
	Q_OBJECT

public:
	enum State {
		None = QXmppMessage::None,
		Active = QXmppMessage::Active,
		Inactive = QXmppMessage::Inactive,
		Gone = QXmppMessage::Gone,
		Composing = QXmppMessage::Composing,
		Paused = QXmppMessage::Paused
	};
	Q_ENUM(State)
};

Q_DECLARE_METATYPE(ChatState::State)

struct DisplayedMessageReaction
{
	Q_GADGET
	Q_PROPERTY(QString emoji MEMBER emoji)
	Q_PROPERTY(int count MEMBER count)
	Q_PROPERTY(bool ownReactionIncluded MEMBER ownReactionIncluded)
	Q_PROPERTY(MessageReactionDeliveryState::Enum deliveryState MEMBER deliveryState)

public:
	QString emoji;
	int count;
	bool ownReactionIncluded;
	MessageReactionDeliveryState::Enum deliveryState;

	bool operator<(const DisplayedMessageReaction &other) const;
};

Q_DECLARE_METATYPE(DisplayedMessageReaction)

struct DetailedMessageReaction
{
	Q_GADGET
	Q_PROPERTY(QString senderJid MEMBER senderJid)
	Q_PROPERTY(QStringList emojis MEMBER emojis)

public:
	QString senderJid;
	QStringList emojis;

	bool operator<(const DetailedMessageReaction &other) const;
};

Q_DECLARE_METATYPE(DetailedMessageReaction)

class MessageModel : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(QString currentAccountJid READ currentAccountJid NOTIFY currentAccountJidChanged)
	Q_PROPERTY(QString currentChatJid READ currentChatJid NOTIFY currentChatJidChanged)
	Q_PROPERTY(QString currentDraftMessageId READ currentDraftMessageId NOTIFY currentDraftMessageIdChanged)
	Q_PROPERTY(bool isOmemoEncryptionEnabled READ isOmemoEncryptionEnabled NOTIFY isOmemoEncryptionEnabledChanged)
	Q_PROPERTY(Encryption::Enum encryption READ encryption WRITE setEncryption NOTIFY encryptionChanged)
	Q_PROPERTY(QList<QString> usableOmemoDevices READ usableOmemoDevices NOTIFY usableOmemoDevicesChanged)
	Q_PROPERTY(QXmppMessage::State chatState READ chatState NOTIFY chatStateChanged)
	Q_PROPERTY(bool mamLoading READ mamLoading NOTIFY mamLoadingChanged)

public:
	// Basically copy from QXmpp, but we need to expose this to QML
	enum MessageRoles {
		Timestamp = Qt::UserRole + 1,
		Id,
		Sender,
		Recipient,
		Encryption,
		IsTrusted,
		Body,
		IsOwn,
		IsEdited,
		DeliveryState,
		IsLastRead,
		IsSpoiler,
		SpoilerHint,
		ErrorText,
		DeliveryStateIcon,
		DeliveryStateName,
		Files,
		DisplayedReactions,
		DetailedReactions,
		OwnDetailedReactions,
	};
	Q_ENUM(MessageRoles)

	static MessageModel *instance();

	MessageModel(QObject *parent = nullptr);
	~MessageModel();

	Q_REQUIRED_RESULT bool isEmpty() const;
	Q_REQUIRED_RESULT int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	Q_REQUIRED_RESULT QHash<int, QByteArray> roleNames() const override;
	Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role) const override;

	Q_INVOKABLE void fetchMore(const QModelIndex &parent) override;
	Q_INVOKABLE bool canFetchMore(const QModelIndex &parent) const override;

	QString currentAccountJid();
	QString currentChatJid();
	Q_INVOKABLE void setCurrentChat(const QString &accountJid, const QString &chatJid);
	Q_INVOKABLE void resetCurrentChat();

	QString currentDraftMessageId() const;

	/**
	 * Determines whether a chat is the currently open chat.
	 *
	 * @param accountJid JID of the chat's account
	 * @param chatJid JID of the chat
	 *
	 * @return true if the chat is currently open, otherwise false
	 */
	bool isChatCurrentChat(const QString &accountJid, const QString &chatJid) const;

	QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> keys();

	Encryption::Enum activeEncryption();
	bool isOmemoEncryptionEnabled() const;

	Encryption::Enum encryption() const;
	void setEncryption(Encryption::Enum encryption);

	QList<QString> usableOmemoDevices() const;

	Q_INVOKABLE void resetComposingChatState();

	Q_INVOKABLE void handleMessageRead(int readMessageIndex);
	Q_INVOKABLE int firstUnreadContactMessageIndex();
	void updateLastReadOwnMessageId();
	Q_INVOKABLE void markMessageAsFirstUnread(int index);

	/**
	 * Adds a message reaction.
	 *
	 * It should only be called if the reaction to be added is not yet stored.
	 *
	 * @param messageId ID of the message to react to
	 * @param emoji emoji in reaction to the message
	 */
	Q_INVOKABLE void addMessageReaction(const QString &messageId, const QString &emoji);

	/**
	 * Removes a message reaction.
	 *
	 * It must only be called if the reaction to be removed is already stored.
	 *
	 * @param messageId ID of the message to react to
	 * @param emoji emoji in reaction to the message
	 */
	Q_INVOKABLE void removeMessageReaction(const QString &messageId, const QString &emoji);

	/**
	 * Sends message reactions again in case their delivery failed.
	 *
	 * It should only be called if there is at least one message reaction with the deliveryState
	 * "ErrorOnAddition", "ErrorOnRemovalAfterSent" or "ErrorOnRemovalAfterDelivered".
	 *
	 * @param messageId ID of the message to react to
	 */
	Q_INVOKABLE void resendMessageReactions(const QString &messageId);

	void sendPendingMessageReactions(const QString &accountJid);

	Q_INVOKABLE bool canCorrectMessage(int index) const;

	/**
	 * Correct the last message
	 */
	Q_INVOKABLE void correctMessage(const QString &msgId, const QString &message);

	/**
	 * Removes a message locally.
	 * 
	 * @param messageId ID of the message
	 */
	Q_INVOKABLE void removeMessage(const QString &messageId);

	/**
	 * Searches from the most recent to the oldest message to find a given substring (case insensitive).
	 *
	 * If no index is passed, the search begins from the most recent message.
	 *
	 * @param searchString substring to search for
	 * @param startIndex index of the first message to search for the given string
	 *
	 * @return index of the first found message containing the given string or -1 if no message containing the given string could be found
	 */
	Q_INVOKABLE int searchForMessageFromNewToOld(const QString &searchString, int startIndex = 0);

	/**
	 * Searches from the oldest to the most recent message to find a given substring (case insensitive).
	 *
	 * If no index is passed, the search begins from the oldest message.
	 *
	 * @param searchString substring to search for
	 * @param startIndex index of the first message to search for the given string
	 *
	 * @return index of the first found message containing the given string or -1 if no message containing the given string could be found
	 */
	Q_INVOKABLE int searchForMessageFromOldToNew(const QString &searchString, int startIndex = -1);

	/**
	 * Sends pending messages again after searching them in the database.
	 */
	void sendPendingMessages();

	/**
	  * Returns the current chat state
	  */
	QXmppMessage::State chatState() const;

	/**
	  * Sends the chat state notification
	  */
	void sendChatState(QXmppMessage::State state);
	Q_INVOKABLE void sendChatState(ChatState::State state);

	bool mamLoading() const;
	void setMamLoading(bool mamLoading);

signals:
	void currentAccountJidChanged(const QString &accountJid);
	void currentChatJidChanged(const QString &currentChatJid);
	void currentDraftMessageIdChanged(const QString &id);

	void isOmemoEncryptionEnabledChanged();
	void encryptionChanged();
	void usableOmemoDevicesChanged();

	void mamLoadingChanged();
	void messageFetchingFinished();

	void keysRetrieved(const QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> &keys);
	void keysChanged();

	void pendingMessagesFetched(const QVector<Message> &messages);
	void sendCorrectedMessageRequested(const Message &msg);
	void chatStateChanged();
	void sendChatStateRequested(const QString &bareJid, QXmppMessage::State state);
	void handleChatStateRequested(const QString &bareJid, QXmppMessage::State state);
	void mamBacklogRetrieved(const QString &accountJid, const QString &jid, const QDateTime &lastStamp, bool complete);

	/**
	 * Emitted to remove all messages of an account or an account's chat.
	 *
	 * @param accountJid JID of the account whose messages are being removed
	 * @param chatJid JID of the chat whose messages are being removed (optional)
	 */
	void removeMessagesRequested(const QString &accountJid, const QString &chatJid = {});

	/**
	 * Emitted when fetching messages for a query string is completed.
	 *
	 * @param queryStringMessageIndex message index of found query string
	 */
	void messageSearchFinished(int queryStringMessageIndex);

private:
	void handleKeysRetrieved(const QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> &keys);

	void handleMessagesFetched(const QVector<Message> &m_messages);
	void handleMamBacklogRetrieved(const QString &accountJid, const QString &jid, const QDateTime &lastStamp, bool complete);

	void addMessage(const Message &msg);
	void updateMessage(const QString &id,
	                   const std::function<void (Message &)> &updateMsg);

	void handleMessage(Message msg, MessageOrigin origin);
	void handleChatState(const QString &bareJid, QXmppMessage::State state);
	void handleMessageRemoved(const QString &senderJid, const QString &recipientJid, const QString &messageId);

	void resetCurrentChat(const QString &accountJid, const QString &chatJid);

	/**
	 * Removes all messages of an account or an account's chat.
	 *
	 * @param accountJid JID of the account whose messages are being removed
	 * @param chatJid JID of the chat whose messages are being removed (optional)
	 */
	void removeMessages(const QString &accountJid, const QString &chatJid = {});

	void removeAllMessages();

	void insertMessage(int i, const Message &msg);

	/**
	 * Shortens messages to 10000 if longer to prevent DoS
	 * @param message to process
	 */
	void processMessage(Message &msg);

	/**
	 * Shows a notification for the message when needed
	 *
	 * @param message message for which a notification might be shown
	 */
	void showMessageNotification(const Message &message, MessageOrigin origin) const;

	void updateMessageReactionsAfterSending(const QString &messageId, const QString &senderJid);

	QVector<Message> m_messages;
	QString m_currentAccountJid;
	QString m_currentChatJid;
	RosterItemWatcher m_rosterItemWatcher;
	UserResourcesWatcher m_contactResourcesWatcher;
	OmemoWatcher m_accountOmemoWatcher;
	OmemoWatcher m_contactOmemoWatcher;
	QString m_lastReadOwnMessageId;
	bool m_fetchedAllFromDb = false;
	bool m_fetchedAllFromMam = false;
	bool m_mamLoading = false;
	QDateTime m_mamBacklogLastStamp;

	QXmppMessage::State m_chatPartnerChatState = QXmppMessage::State::None;
	QXmppMessage::State m_ownChatState = QXmppMessage::State::None;
	QTimer *m_composingTimer;
	QTimer *m_stateTimeoutTimer;
	QTimer *m_inactiveTimer;
	QTimer *m_chatPartnerChatStateTimeout;
	QMap<QString, QXmppMessage::State> m_chatStateCache;

	QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> m_keys;

	static MessageModel *s_instance;
};

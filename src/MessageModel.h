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
// Kaidan
#include "Message.h"

class GroupChatUser;
class QTimer;
class Kaidan;

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
	QString senderId;
	QString senderJid;
	QString senderName;
	QStringList emojis;

	bool operator==(const DetailedMessageReaction &other) const = default;
	bool operator<(const DetailedMessageReaction &other) const;
};

Q_DECLARE_METATYPE(DetailedMessageReaction)

class MessageModel : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(bool mamLoading READ mamLoading NOTIFY mamLoadingChanged)

public:
	enum MessageRoles {
		SenderJid = Qt::UserRole + 1,
		GroupChatSenderId,
		SenderName,
		Id,
		IsLastReadOwnMessage,
		IsLastReadContactMessage,
		IsEdited,
		ReplyToJid,
		ReplyToGroupChatParticipantId,
		ReplyToName,
		ReplyId,
		ReplyQuote,
		Date,
		NextDate,
		Time,
		Body,
		Encryption,
		TrustLevel,
		DeliveryState,
		DeliveryStateIcon,
		DeliveryStateName,
		IsSpoiler,
		SpoilerHint,
		IsOwn,
		Files,
		DisplayedReactions,
		DetailedReactions,
		OwnReactionsFailed,
		GroupChatInvitationJid,
		ErrorText,
	};
	Q_ENUM(MessageRoles)

	static MessageModel *instance();

	MessageModel(QObject *parent = nullptr);
	~MessageModel();

	[[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;
	[[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

	Q_INVOKABLE void fetchMore(const QModelIndex &parent) override;
	Q_SIGNAL void messageFetchingFinished();
	Q_INVOKABLE bool canFetchMore(const QModelIndex &parent) const override;

	Q_INVOKABLE void handleMessageRead(int readMessageIndex);
	Q_INVOKABLE int firstUnreadContactMessageIndex();
	void updateLastReadOwnMessageId();
	Q_INVOKABLE void markMessageAsFirstUnread(int index);

	/**
	 * Adds a message reaction or undoes its pending/failed removal.
	 *
	 * @param messageId ID of the message to react to
	 * @param emoji emoji in reaction to the message
	 */
	Q_INVOKABLE void addMessageReaction(const QString &messageId, const QString &emoji);

	/**
	 * Removes a message reaction or undoes its pending/failed addition.
	 *
	 * This must only be called if the reaction to be removed is already stored.
	 *
	 * @param messageId ID of the message to react to
	 * @param emoji emoji in reaction to the message
	 */
	Q_INVOKABLE void removeMessageReaction(const QString &messageId, const QString &emoji);

	/**
	 * Sends message reactions again in case their delivery failed.
	 *
	 * This should only be called if there is at least one message reaction with the deliveryState
	 * "ErrorOnAddition", "ErrorOnRemovalAfterSent" or "ErrorOnRemovalAfterDelivered".
	 *
	 * @param messageId ID of the message to react to
	 */
	Q_INVOKABLE void resendMessageReactions(const QString &messageId);

	Q_INVOKABLE bool canCorrectMessage(int index) const;

	/**
	 * Removes a message locally.
	 * 
	 * @param messageId ID of the message
	 */
	Q_INVOKABLE void removeMessage(const QString &messageId);

	void removeAllMessages();

	/**
	 * Searches a message locally by its ID.
	 *
	 * @param messageId ID of the message
	 *
	 * @return index of the found message, otherwise -1
	 */
	Q_INVOKABLE int searchMessageById(const QString &messageId);

	/**
	 * Emitted when searching a message in the DB and if found, fetching all messages until the
	 * found one, is finished.
	 *
	 * @param foundMessageIndex index of the found message, otherwise -1
	 */
	Q_SIGNAL void messageSearchByIdInDbFinished(int foundMessageIndex);

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
	 * Emitted when fetching messages for a query string is completed.
	 *
	 * @param queryStringMessageIndex message index of found query string
	 */
	Q_SIGNAL void messageSearchFinished(int queryStringMessageIndex);

	bool mamLoading() const;
	void setMamLoading(bool mamLoading);
	Q_SIGNAL void mamLoadingChanged();

private:
	void handleMessagesFetched(const QVector<Message> &m_messages);
	void handleMamBacklogRetrieved(bool complete);

	void handleMessage(Message msg, MessageOrigin origin);
	void handleMessageUpdated(Message message);

	void handleDevicesChanged(const QString &accountJid, QList<QString> jids);

	void addMessage(const Message &msg);
	void insertMessage(int i, const Message &msg);

	/**
	 * Removes all messages of an account or an account's chat.
	 *
	 * @param accountJid JID of the account whose messages are being removed
	 * @param chatJid JID of the chat whose messages are being removed (optional)
	 */
	void removeMessages(const QString &accountJid, const QString &chatJid = {});

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
	void showMessageNotification(const Message &message, MessageOrigin origin);

	/**
	 * Undoes a pending or failed removal of a message reaction.
	 *
	 * This should only be called if the removal of the reaction could not be submitted (yet).
	 * That is either the case if Kaidan has not been connected since the user removed the reaction
	 * or when there was an error on sending the change.
	 *
	 * @param messageId ID of the message to react to
	 * @param senderJid JID of the reaction's sender
	 * @param emoji emoji in reaction to the message
	 * @param reactions message's reactions to be updated
	 *
	 * @return whether the message reaction's removal has been undone
	 */
	bool undoMessageReactionRemoval(const QString &messageId, const QString &senderJid, const QString &emoji, const QVector<MessageReaction> &reactions);

	/**
	 * Undoes a pending or failed addition of a message reaction.
	 *
	 * This should only be called if the addition of the reaction could not be submitted (yet).
	 * That is either the case if Kaidan has not been connected since the user added the reaction
	 * or when there was an error on sending the change.
	 *
	 * @param messageId ID of the message to react to
	 * @param senderJid JID of the reaction's sender
	 * @param emoji emoji in reaction to the message
	 * @param reactions message's reactions to be updated
	 *
	 * @return whether the message reaction's addition has been undone
	 */
	bool undoMessageReactionAddition(const QString &messageId, const QString &senderJid, const QString &emoji, const QVector<MessageReaction> &reactions);

	/**
	 * Searches a message with a more recent date and returns that date.
	 *
	 * This is needed as a workaround for a bug in Qt Quick's implementation of the ListView section
	 * for "verticalLayoutDirection: ListView.BottomToTop".
	 * That bug results in each section label being displayed at the bottom of its corresponding
	 * section instead of displaying it at the top of it.
	 *
	 * @param messageStartIndex index of the message to start the search with
	 *
	 * @return the date of the first message with a more recent date
	 */
	QDate searchNextDate(int messageStartIndex) const;

	QString formatDate(QDate localDate) const;
	QString determineGroupChatSenderName(const Message &message) const;
	QString determineReplyToName(const Message::Reply &reply) const;

	QVector<Message> m_messages;
	QString m_lastReadOwnMessageId;
	QString m_lastReadContactMessageId;
	bool m_fetchedAllFromDb = false;
	bool m_fetchedAllFromMam = false;
	bool m_mamLoading = false;

	static MessageModel *s_instance;
};

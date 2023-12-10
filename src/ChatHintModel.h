// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemModel>

class ChatHintButton {
	Q_GADGET

	Q_PROPERTY(ChatHintButton::Type type MEMBER type)
	Q_PROPERTY(QString text MEMBER text)

public:
	enum Type {
		Dismiss,
		ConnectToServer,
		AllowPresenceSubscription,
	};
	Q_ENUM(Type)

	Type type;
	QString text;

	bool operator==(const ChatHintButton &other) const = default;
};

Q_DECLARE_METATYPE(ChatHintButton)
Q_DECLARE_METATYPE(ChatHintButton::Type)

class MessageModel;

class ChatHintModel : public QAbstractListModel
{
	Q_OBJECT

public:
	enum class Role {
		Text = Qt::UserRole,
		Buttons,
		Loading,
		LoadingDescription,
	};

	static ChatHintModel *instance();

	ChatHintModel(QObject *parent = nullptr);
	~ChatHintModel();

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QHash<int, QByteArray> roleNames() const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	Q_INVOKABLE void handleButtonClicked(int i, ChatHintButton::Type type);

	Q_SIGNAL void presenceSubscriptionRequestReceivedRequested(const QString &accountJid, const QString &subscriberJid, const QString &requestText);

private:
	struct ChatHint {
		QString text;
		QVector<ChatHintButton> buttons;
		bool loading = false;
		QString loadingDescription;

		bool operator==(const ChatHint &other) const = default;
	};

	void handleConnectionStateChanged();
	void handleConnectionErrorChanged();
	void handleConnectionErrorChanged(int i);

	void handleRosterItemPresenceSubscription();
	void handleUnrespondedPresenceSubscriptionRequests();
	void handlePresenceSubscriptionRequestReceived(const QString &accountJid, const QString &subscriberJid, const QString &requestText);

	int addConnectToServerChatHint(bool loading = false);
	int addAllowPresenceSubscriptionChatHint(const QString &requestText);

	int addChatHint(const ChatHint &chatHint);
	void insertChatHint(int i, const ChatHint &chatHint);
	void updateChatHint(ChatHintButton::Type buttonType, const std::function<void (ChatHint &)> &updateChatHint);
	void updateChatHint(int i, const std::function<void (ChatHint &)> &updateChatHint);
	void removeChatHint(ChatHintButton::Type buttonType);
	void removeChatHint(int i);

	int chatHintIndex(ChatHintButton::Type buttonType) const;
	bool hasButton(int i, ChatHintButton::Type buttonType) const;

	MessageModel *m_messageModel;
	QVector<ChatHint> m_chatHints;

	static ChatHintModel *s_instance;
};

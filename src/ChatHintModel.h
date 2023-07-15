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
	};
	Q_ENUM(Type)

	Type type;
	QString text;

	bool operator==(const ChatHintButton &other) const = default;
};

Q_DECLARE_METATYPE(ChatHintButton)
Q_DECLARE_METATYPE(ChatHintButton::Type)

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

	explicit ChatHintModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QHash<int, QByteArray> roleNames() const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	Q_INVOKABLE void handleButtonClicked(int index, ChatHintButton::Type type);

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

	void addChatHint(const ChatHint &chatHint);
	void insertChatHint(int index, const ChatHint &chatHint);
	bool updateChatHint(ChatHintButton::Type buttonType, const std::function<void (ChatHint &)> &updateChatHint);
	void removeChatHint(int index);

	int chatHintIndex(ChatHintButton::Type buttonType) const;
	bool hasButton(int index, ChatHintButton::Type buttonType) const;

	QVector<ChatHint> m_chatHints;
};

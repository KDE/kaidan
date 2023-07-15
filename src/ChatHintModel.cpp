// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChatHintModel.h"

#include "Kaidan.h"
#include "QmlUtils.h"

ChatHintModel::ChatHintModel(QObject *parent)
	: QAbstractListModel(parent)
{
	connect(Kaidan::instance(), &Kaidan::connectionStateChanged, this, &ChatHintModel::handleConnectionStateChanged);
	connect(Kaidan::instance(), &Kaidan::connectionErrorChanged, this, &ChatHintModel::handleConnectionErrorChanged);
	handleConnectionStateChanged();
}

int ChatHintModel::rowCount(const QModelIndex &) const
{
	return m_chatHints.size();
}

QHash<int, QByteArray> ChatHintModel::roleNames() const
{
	return {
		{ static_cast<int>(Role::Text), QByteArrayLiteral("text") },
		{ static_cast<int>(Role::Buttons), QByteArrayLiteral("buttons") },
		{ static_cast<int>(Role::Loading), QByteArrayLiteral("loading") },
		{ static_cast<int>(Role::LoadingDescription), QByteArrayLiteral("loadingDescription") },
	};
}

QVariant ChatHintModel::data(const QModelIndex &index, int role) const
{
	if (!hasIndex(index.row(), index.column(), index.parent())) {
		qWarning() << "Could not get data from ChatHintModel:" << index << role;
		return {};
	}

	const auto &chatHint = m_chatHints.at(index.row());
	switch (static_cast<Role>(role)) {
	case Role::Text:
		return chatHint.text;
	case Role::Buttons:
		return QVariant::fromValue(chatHint.buttons);
	case Role::Loading:
		return chatHint.loading;
	case Role::LoadingDescription:
		return chatHint.loadingDescription;
	}

	qWarning("[ChatHintModel] Invalid role requested!");
	return {};
}

void ChatHintModel::handleButtonClicked(int index, ChatHintButton::Type type)
{
	switch(type) {
	case ChatHintButton::Dismiss:
		removeChatHint(index);
		return;
	case ChatHintButton::ConnectToServer:
		updateChatHint(ChatHintButton::ConnectToServer, [](ChatHint &chatHint) {
			chatHint.loading = true;
		});
		Kaidan::instance()->logIn();
		return;
	default:
		return;
	}
}

void ChatHintModel::handleConnectionStateChanged()
{
	switch (Enums::ConnectionState(Kaidan::instance()->connectionState())) {
	case Enums::ConnectionState::StateDisconnected:
		if (updateChatHint(ChatHintButton::ConnectToServer, [](ChatHint &chatHint) {
			chatHint.loading = false;
		})) {
			handleConnectionErrorChanged();
			return;
		}

		addChatHint(
			ChatHint {
				tr("You are not connected - Messages are sent and received once connected"),
				{ ChatHintButton { ChatHintButton::Dismiss, tr("Dismiss") },
				  ChatHintButton { ChatHintButton::ConnectToServer, tr("Connect") } },
				false,
				tr("Connecting…"),
			}
		);
		handleConnectionErrorChanged();

		return;
	case Enums::ConnectionState::StateConnecting:
		if (updateChatHint(ChatHintButton::ConnectToServer, [](ChatHint &chatHint) {
			chatHint.loading = true;
		})) {
			return;
		}

		addChatHint(
			ChatHint {
				tr("You are not connected - Messages are sent and received once connected"),
				{ ChatHintButton { ChatHintButton::Dismiss, tr("Dismiss") },
				  ChatHintButton { ChatHintButton::ConnectToServer, tr("Connect") } },
				true,
				tr("Connecting…"),
			}
		);

		return;
	case Enums::ConnectionState::StateConnected:
		if (const auto i = chatHintIndex(ChatHintButton::ConnectToServer); i != -1) {
			removeChatHint(i);
			return;
		}
		return;
	default:
		return;
	}
}

void ChatHintModel::handleConnectionErrorChanged()
{
	if (const auto i = chatHintIndex(ChatHintButton::ConnectToServer); i != -1) {
		if (const auto error = ClientWorker::ConnectionError(Kaidan::instance()->connectionError()); error != ClientWorker::NoError) {
			updateChatHint(ChatHintButton::ConnectToServer, [errorMessage = QmlUtils::connectionErrorMessage(error)](ChatHint &chatHint) {
				chatHint.text = errorMessage;
			});
		}
	}
}

void ChatHintModel::addChatHint(const ChatHint &chatHint)
{
	insertChatHint(m_chatHints.size(), chatHint);
}

void ChatHintModel::insertChatHint(int index, const ChatHint &chatHint)
{
	beginInsertRows(QModelIndex(), index, index);
	m_chatHints.insert(index, chatHint);
	endInsertRows();
}

bool ChatHintModel::updateChatHint(ChatHintButton::Type buttonType, const std::function<void (ChatHint &)> &updateChatHint)
{
	if (const auto i = chatHintIndex(buttonType); i != -1) {
		auto chatHint = m_chatHints.at(i);
		updateChatHint(chatHint);

		// Skip further processing in case of no changes.
		if (m_chatHints.at(i) == chatHint) {
			return true;
		}

		m_chatHints.replace(i, chatHint);
		Q_EMIT dataChanged(index(i), index(i));
		return true;
	}

	return false;
}

void ChatHintModel::removeChatHint(int index)
{
	beginRemoveRows(QModelIndex(), index, index);
	m_chatHints.remove(index);
	endRemoveRows();
}

int ChatHintModel::chatHintIndex(ChatHintButton::Type buttonType) const
{
	for (int i = 0; i < m_chatHints.size(); ++i) {
		if (hasButton(i, buttonType)) {
			return i;
		}
	}

	return -1;
}

bool ChatHintModel::hasButton(int index, ChatHintButton::Type buttonType) const
{
	const auto buttons = m_chatHints.at(index).buttons;

	return std::any_of(buttons.cbegin(), buttons.cend(), [buttonType](const ChatHintButton &button) {
		return button.type == buttonType;
	});
}

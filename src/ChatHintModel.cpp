// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChatHintModel.h"

// Qt
#include <QGuiApplication>
// Kaidan
#include "FutureUtils.h"
#include "Kaidan.h"
#include "MessageModel.h"
#include "Notifications.h"
#include "QmlUtils.h"
#include "RosterManager.h"

ChatHintModel *ChatHintModel::s_instance = nullptr;

ChatHintModel *ChatHintModel::instance()
{
	return s_instance;
}

ChatHintModel::ChatHintModel(QObject *parent)
	: QAbstractListModel(parent), m_messageModel(MessageModel::instance())
{
	Q_ASSERT(!s_instance);
	s_instance = this;

	connect(Kaidan::instance(), &Kaidan::connectionStateChanged, this, &ChatHintModel::handleConnectionStateChanged);
	connect(Kaidan::instance(), &Kaidan::connectionErrorChanged, this, qOverload<>(&ChatHintModel::handleConnectionErrorChanged));

	connect(this, &ChatHintModel::presenceSubscriptionRequestReceivedRequested, this, &ChatHintModel::handlePresenceSubscriptionRequestReceived);
	connect(&m_messageModel->rosterItemWatcher(), &RosterItemWatcher::itemChanged, this, &ChatHintModel::handleRosterItemPresenceSubscription);

	connect(Kaidan::instance(), &Kaidan::openChatPageRequested, this, [this]() {
		handleConnectionStateChanged();
		handleUnrespondedPresenceSubscriptionRequests();
	});
}

ChatHintModel::~ChatHintModel() = default;

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

void ChatHintModel::handleButtonClicked(int i, ChatHintButton::Type type)
{
	switch(type) {
	case ChatHintButton::Dismiss:
		if (i == chatHintIndex(ChatHintButton::AllowPresenceSubscription)) {
			Kaidan::instance()->client()->rosterManager()->refuseSubscriptionToPresenceRequested(m_messageModel->currentChatJid());
		}

		removeChatHint(i);
		return;
	case ChatHintButton::ConnectToServer:
		updateChatHint(i, [](ChatHint &chatHint) {
			chatHint.loading = true;
		});
		Kaidan::instance()->logIn();
		return;
	case ChatHintButton::AllowPresenceSubscription:
		updateChatHint(i, [](ChatHint &chatHint) {
			chatHint.loading = true;
		});
		Kaidan::instance()->client()->rosterManager()->acceptSubscriptionToPresenceRequested(m_messageModel->currentChatJid());
		return;
	default:
		return;
	}
}

void ChatHintModel::handleConnectionStateChanged()
{
	switch (Enums::ConnectionState(Kaidan::instance()->connectionState())) {
	case Enums::ConnectionState::StateDisconnected:
		if (const auto i = chatHintIndex(ChatHintButton::ConnectToServer) == -1) {
			handleConnectionErrorChanged(addConnectToServerChatHint(false));
		} else {
			updateChatHint(i, [](ChatHint &chatHint) {
				chatHint.loading = false;
			});

			handleConnectionErrorChanged(i);
		}

		return;
	case Enums::ConnectionState::StateConnecting:
		if (const auto i = chatHintIndex(ChatHintButton::ConnectToServer) == -1) {
			addConnectToServerChatHint(true);
		} else {
			updateChatHint(i, [](ChatHint &chatHint) {
				chatHint.loading = true;
			});
		}

		return;
	case Enums::ConnectionState::StateConnected:
		removeChatHint(ChatHintButton::ConnectToServer);
		return;
	default:
		return;
	}
}

void ChatHintModel::handleConnectionErrorChanged()
{
	updateChatHint(ChatHintButton::ConnectToServer, [](ChatHint &chatHint) {
		if (const auto error = ClientWorker::ConnectionError(Kaidan::instance()->connectionError()); error != ClientWorker::NoError) {
			chatHint.text = QmlUtils::connectionErrorMessage(error);
		}
	});
}

void ChatHintModel::handleConnectionErrorChanged(int i)
{
	updateChatHint(i, [](ChatHint &chatHint) {
		if (const auto error = ClientWorker::ConnectionError(Kaidan::instance()->connectionError()); error != ClientWorker::NoError) {
			chatHint.text = QmlUtils::connectionErrorMessage(error);
		}
	});
}

void ChatHintModel::handleRosterItemPresenceSubscription()
{
	const auto subscription = m_messageModel->rosterItemWatcher().item().subscription;
	const auto subscriptionAllowed =
			subscription == QXmppRosterIq::Item::SubscriptionType::From ||
			subscription == QXmppRosterIq::Item::SubscriptionType::Both;

	if (const auto i = chatHintIndex(ChatHintButton::AllowPresenceSubscription); i != -1 && subscriptionAllowed) {
		removeChatHint(i);
		Notifications::instance()->closePresenceSubscriptionRequestNotification(m_messageModel->currentAccountJid(), m_messageModel->currentChatJid());
	}
}

void ChatHintModel::handleUnrespondedPresenceSubscriptionRequests()
{
	const auto rosterManager = Kaidan::instance()->client()->rosterManager();

	runOnThread(rosterManager, [this, rosterManager, accountJid = m_messageModel->currentAccountJid(), chatJid = m_messageModel->currentChatJid()]() {
		if (const auto unrespondedPresenceSubscriptionRequests = rosterManager->unrespondedPresenceSubscriptionRequests();
			unrespondedPresenceSubscriptionRequests.contains(chatJid)) {
			runOnThread(this, [this, requestText = unrespondedPresenceSubscriptionRequests.value(chatJid)]() {
				this->addAllowPresenceSubscriptionChatHint(requestText);
			});
		}
	});
}

void ChatHintModel::handlePresenceSubscriptionRequestReceived(const QString &accountJid, const QString &subscriberJid, const QString &requestText)
{
	bool userMuted = m_messageModel->rosterItemWatcher().item().notificationsMuted;
	bool requestForCurrentChat = subscriberJid == m_messageModel->currentChatJid();
	bool chatActive = requestForCurrentChat && QGuiApplication::applicationState() == Qt::ApplicationActive;

	if (requestForCurrentChat) {
		addAllowPresenceSubscriptionChatHint(requestText);
	}

	if (!userMuted && !chatActive) {
		Notifications::instance()->sendPresenceSubscriptionRequestNotification(accountJid, subscriberJid);
	}
}

int ChatHintModel::addConnectToServerChatHint(bool loading)
{
	return addChatHint(
		ChatHint {
			tr("You are not connected - Messages are sent and received once connected"),
			{ ChatHintButton { ChatHintButton::Dismiss, tr("Dismiss") },
			  ChatHintButton { ChatHintButton::ConnectToServer, tr("Connect") } },
			loading,
			tr("Connecting…"),
		}
	);
}

int ChatHintModel::addAllowPresenceSubscriptionChatHint(const QString &requestText)
{
	const auto displayName = m_messageModel->rosterItemWatcher().item().displayName();
	const auto appendedText = requestText.isEmpty() ? QString() : QString(": %1").arg(requestText);

	return addChatHint(
		ChatHint {
			tr("%1 would like to receive your personal data such as availability, devices and other personal information%2").arg(displayName, appendedText),
			{ ChatHintButton { ChatHintButton::Dismiss, tr("Refuse") },
			  ChatHintButton { ChatHintButton::AllowPresenceSubscription, tr("Allow") } },
			false,
			tr("Allowing…"),
		}
	);
}

int ChatHintModel::addChatHint(const ChatHint &chatHint)
{
	const int i = m_chatHints.size();
	insertChatHint(i, chatHint);
	return i;
}

void ChatHintModel::insertChatHint(int i, const ChatHint &chatHint)
{
	beginInsertRows(QModelIndex(), i, i);
	m_chatHints.insert(i, chatHint);
	endInsertRows();
}

void ChatHintModel::updateChatHint(ChatHintButton::Type buttonType, const std::function<void (ChatHint &)> &updateChatHint)
{
	if (const auto i = chatHintIndex(buttonType) != -1) {
		this->updateChatHint(i, updateChatHint);
	}
}

void ChatHintModel::updateChatHint(int i, const std::function<void (ChatHint &)> &updateChatHint)
{
		auto chatHint = m_chatHints.at(i);
		updateChatHint(chatHint);

		// Skip further processing in case of no changes.
		if (m_chatHints.at(i) == chatHint) {
			return;
		}

		m_chatHints.replace(i, chatHint);
		const auto modelIndex = index(i);
		Q_EMIT dataChanged(modelIndex, modelIndex);
}

void ChatHintModel::removeChatHint(ChatHintButton::Type buttonType)
{
	if (const auto i = chatHintIndex(buttonType); i != -1) {
		removeChatHint(i);
	}
}

void ChatHintModel::removeChatHint(int i)
{
	beginRemoveRows(QModelIndex(), i, i);
	m_chatHints.remove(i);
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

bool ChatHintModel::hasButton(int i, ChatHintButton::Type buttonType) const
{
	const auto buttons = m_chatHints.at(i).buttons;

	return std::any_of(buttons.cbegin(), buttons.cend(), [buttonType](const ChatHintButton &button) {
		return button.type == buttonType;
	});
}

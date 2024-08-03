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

// QXmpp
#include <QXmppMessage.h>
// Kaidan
#include "PresenceCache.h"
#include "RosterItemWatcher.h"

class QTimer;
class EncryptionWatcher;

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

class ChatController : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString accountJid READ accountJid NOTIFY accountJidChanged)
	Q_PROPERTY(QString chatJid READ chatJid NOTIFY chatJidChanged)
	Q_PROPERTY(const RosterItem &rosterItem READ rosterItem NOTIFY rosterItemChanged)
	Q_PROPERTY(EncryptionWatcher *accountEncryptionWatcher READ accountEncryptionWatcher CONSTANT)
	Q_PROPERTY(EncryptionWatcher *chatEncryptionWatcher READ chatEncryptionWatcher CONSTANT)
	Q_PROPERTY(bool isEncryptionEnabled READ isEncryptionEnabled NOTIFY isEncryptionEnabledChanged)
	Q_PROPERTY(Encryption::Enum encryption READ encryption WRITE setEncryption NOTIFY encryptionChanged)
	Q_PROPERTY(QXmppMessage::State chatState READ chatState NOTIFY chatStateChanged)

public:
	static ChatController *instance();

	ChatController(QObject *parent = nullptr);
	~ChatController();

	QString accountJid();
	Q_SIGNAL void accountJidChanged(const QString &accountJid);

	QString chatJid();
	Q_SIGNAL void chatJidChanged(const QString &chatJid);

	Q_INVOKABLE void setChat(const QString &accountJid, const QString &chatJid);
	Q_INVOKABLE void resetChat();
	Q_SIGNAL void chatChanged();

	/**
	 * Determines whether a chat is the currently open chat.
	 *
	 * @param accountJid JID of the chat's account
	 * @param chatJid JID of the chat
	 *
	 * @return true if the chat is currently open, otherwise false
	 */
	bool isChatCurrentChat(const QString &accountJid, const QString &chatJid) const;

	const RosterItem &rosterItem() const;
	Q_SIGNAL void rosterItemChanged();

	EncryptionWatcher *accountEncryptionWatcher() const;
	EncryptionWatcher *chatEncryptionWatcher() const;

	void handleGroupChatUserJidsFetched();

	Encryption::Enum activeEncryption() const;

	bool isEncryptionEnabled() const;
	Q_SIGNAL void isEncryptionEnabledChanged();

	Encryption::Enum encryption() const;
	void setEncryption(Encryption::Enum encryption);
	Q_SIGNAL void encryptionChanged();

	Q_INVOKABLE void resetComposingChatState();

	/**
	  * Returns the current chat state
	  */
	QXmppMessage::State chatState() const;
	Q_SIGNAL void chatStateChanged();

	/**
	  * Sends the chat state notification
	  */
	void sendChatState(QXmppMessage::State state);
	Q_INVOKABLE void sendChatState(ChatState::State state);

	void handleChatState(const QString &bareJid, QXmppMessage::State state);

private:
	bool hasUsableEncryptionDevices() const;
	void resetChat(const QString &accountJid, const QString &chatJid);

	QString m_accountJid;
	QString m_chatJid;
	RosterItemWatcher m_rosterItemWatcher;
	UserResourcesWatcher m_contactResourcesWatcher;
	EncryptionWatcher *m_accountEncryptionWatcher;
	EncryptionWatcher *m_chatEncryptionWatcher;

	QXmppMessage::State m_chatPartnerChatState = QXmppMessage::State::None;
	QXmppMessage::State m_ownChatState = QXmppMessage::State::None;
	QTimer *m_composingTimer;
	QTimer *m_stateTimeoutTimer;
	QTimer *m_inactiveTimer;
	QTimer *m_chatPartnerChatStateTimeout;
	QMap<QString, QXmppMessage::State> m_chatStateCache;

	static ChatController *s_instance;
};

// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef QXMPPURI_H
#define QXMPPURI_H

#include <QUrlQuery>

#include <QXmppMessage.h>

///
/// This class represents an XMPP URI as specified by RFC 5122 -
/// Internationalized Resource Identifiers (IRIs) and Uniform Resource
/// Identifiers (URIs) for the Extensible Messaging and Presence Protocol
/// (XMPP) and XEP-0147: XMPP URI Scheme Query Components.
///
/// A QUrlQuery is used by this class to represent a query (component) of an
/// XMPP URI. A query conisists of query items which can be the query type or a
/// key-value pair.
///
/// A query type is used to perform an action while the key-value pairs are
/// used to define its behavior.
///
/// Example:
/// xmpp:alice@example.org?message;subject=Hello
///
/// query (component): message;subject=Hello;body=world
/// query items: message, subject=Hello, body=world
/// query type: message
/// key-value pair 1: subject=Hello
/// key-value pair 2: body=world
///
class QXmppUri
{
public:
	enum Action {
		None,
		Command,
		Disco,
		Invite,
		Join,
		Login,
		Message,
		PubSub,
		RecvFile,
		Register,
		Remove,
		Roster,
		SendFile,
		Subscribe,
		TrustMessage,
		Unregister,
		Unsubscribe,
		VCard,
	};

	QXmppUri() = default;
	QXmppUri(QString uri);

	QString toString();

	QString jid() const;
	void setJid(const QString &jid);

	Action action() const;
	void setAction(const Action &action);

	// login
	QString password() const;
	void setPassword(const QString &password);

	// message
	QXmppMessage message() const;
	void setMessage(const QXmppMessage&);

	bool hasMessageType() const;
	void setHasMessageType(bool hasMessageType);

	// trust-message
	QString encryption() const;
	void setEncryption(const QString &encryption);
	QList<QString> trustedKeysIds() const;
	void setTrustedKeysIds(const QList<QString> &keyIds);
	QList<QString> distrustedKeysIds() const;
	void setDistrustedKeysIds(const QList<QString> &keyIds);

	static bool isXmppUri(const QString &uri);

private:
	bool setAction(const QUrlQuery &query);
	QString queryType() const;
	void setQueryKeyValuePairs(const QUrlQuery &query);
	void addItemsToQuery(QUrlQuery &query) const;

	static void addKeyValuePairToQuery(QUrlQuery &query, const QString &key, QStringView val);
	static QString queryItemValue(const QUrlQuery &query, const QString &key);

	QString m_jid;
	Action m_action = None;

	// login
	QString m_password;

	// message
	QXmppMessage m_message;
	bool m_hasMessageType = false;

	// trust-message
	QString m_encryption;
	QList<QString> m_trustedKeysIds;
	QList<QString> m_distrustedKeysIds;
};

#endif // QXMPPURI_H

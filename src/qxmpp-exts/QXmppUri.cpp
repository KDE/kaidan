// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QXmppUri.h"

#include <QUrlQuery>

#include <array>

constexpr QStringView SCHEME = u"xmpp";
constexpr QStringView PREFIX = u"xmpp:";
constexpr QChar QUERY_ITEM_DELIMITER = u';';
constexpr QChar QUERY_ITEM_KEY_DELIMITER = u'=';

// Query types representing actions, e.g. "join" in
// "xmpp:group@example.org?join" for joining a group chat
constexpr std::array<QStringView, 18> QUERY_TYPES = {
	QStringView(),
	u"command",
	u"disco",
	u"invite",
	u"join",
	u"login",
	u"message",
	u"pubsub",
	u"recvfile",
	u"register",
	u"remove",
	u"roster",
	u"sendfile",
	u"subscribe",
	u"trust-message",
	u"unregister",
	u"unsubscribe",
	u"vcard"
};

// QXmppMessage types as strings
constexpr std::array<QStringView, 5> MESSAGE_TYPES = {
	u"error",
	u"normal",
	u"chat",
	u"groupchat",
	u"headline"
};

///
/// Parses the URI from a string.
///
/// @param input string which may present an XMPP URI
///
QXmppUri::QXmppUri(QString input)
{
	QUrl url(input);
	if (!url.isValid() || url.scheme() != SCHEME)
		return;

	m_jid = url.path();

	if (!url.hasQuery())
		return;

	QUrlQuery query;
	query.setQueryDelimiters(QUERY_ITEM_KEY_DELIMITER, QUERY_ITEM_DELIMITER);
	query.setQuery(url.query(QUrl::FullyEncoded));

	if (!setAction(query))
		return;

	setQueryKeyValuePairs(query);
}

///
/// Decodes this URI to a string.
///
/// @return this URI as a string
///
QString QXmppUri::toString()
{
	QUrl url;
	url.setScheme(SCHEME.toString());
	url.setPath(m_jid);

	QUrlQuery query;
	query.setQueryDelimiters(QUERY_ITEM_KEY_DELIMITER, QUERY_ITEM_DELIMITER);

	// Add the query item.
	if (m_action != None) {
		addItemsToQuery(query);
	}

	url.setQuery(query);

	return QString::fromUtf8(url.toEncoded(QUrl::FullyEncoded));
}

///
/// Returns the JID this URI is about.
///
/// This can also be e.g. a MUC room in case of a Join action.
///
QString QXmppUri::jid() const
{
	return m_jid;
}

///
/// Sets the JID this URI links to.
///
/// @param jid JID to be set
///
void QXmppUri::setJid(const QString &jid)
{
	m_jid = jid;
}

///
/// Returns the action of this URI.
///
/// This is None in case no action is included.
///
QXmppUri::Action QXmppUri::action() const
{
	return m_action;
}

///
/// Sets the action of this URI, e.g. Join for a URI with the query type
/// \c ?join".
///
/// @param action action to be set
///
void QXmppUri::setAction(const Action &action)
{
	m_action = action;
}

///
/// Returns the password of a login action
///
QString QXmppUri::password() const
{
	return m_password;
}

///
/// Sets the password of a login action.
///
/// @param password
///
void QXmppUri::setPassword(const QString &password)
{
	m_password = password;
}

///
/// In case the URI has a message query, this can be used to get the attached
/// message content directly as \c QXmppMessage.
///
QXmppMessage QXmppUri::message() const
{
	return m_message;
}

///
/// Sets the attached message for a Message action.
///
/// Supported properties are: body, from, id, thread, type, subject.
/// If you want to include the message type, ensure that \c hasMessageType is
/// set to true.
///
/// @param message message to be set
///
void QXmppUri::setMessage(const QXmppMessage &message)
{
	m_action = Message;
	m_message = message;
}

///
/// Returns true, if the attached message's type is included.
///
/// This is required because \c QXmppMessage has no option to set no type.
///
bool QXmppUri::hasMessageType() const
{
	return m_hasMessageType;
}

///
/// Sets whether to include the message's type.
///
/// This is required because \c QXmppMessage has no option to set an empty
/// type.
///
/// @param hasMessageType true if the message's type should be included
///
void QXmppUri::setHasMessageType(bool hasMessageType)
{
	m_hasMessageType = hasMessageType;
}

///
/// Returns the namespace of the encryption protocol that uses the keys
/// corresponding to their identifiers for a trust message action.
///
QString QXmppUri::encryption() const
{
	return m_encryption;
}

///
/// Sets the namespace of the encryption protocol that uses the keys
/// corresponding to their identifiers for a trust message action.
///
/// \param encryption namespace of the encryption protocol
///
void QXmppUri::setEncryption(const QString &encryption)
{
	m_encryption = encryption;
}

///
/// Returns the identifiers of the trusted keys for a trust message action.
///
QList<QString> QXmppUri::trustedKeysIds() const
{
	return m_trustedKeysIds;
}

///
/// Sets the identifiers of the trusted keys for a trust message action.
///
/// \param keyIds identifiers of the trusted keys
///
void QXmppUri::setTrustedKeysIds(const QList<QString> &keyIds)
{
	m_trustedKeysIds = keyIds;
}

///
/// Returns the identifiers of the distrusted keys for a trust message action.
///
QList<QString> QXmppUri::distrustedKeysIds() const
{
	return m_distrustedKeysIds;
}

///
/// Sets the identifiers of the distrusted keys for a trust message action.
///
/// \param keyIds identifiers of the distrusted keys
///
void QXmppUri::setDistrustedKeysIds(const QList<QString> &keyIds)
{
	m_distrustedKeysIds = keyIds;
}

///
/// Checks whether the string starts with the XMPP scheme.
///
/// @param uri URI to check for XMPP scheme
///
bool QXmppUri::isXmppUri(const QString &uri)
{
	return uri.startsWith(PREFIX);
}

///
/// Sets the action represented by its query type.
///
/// The query item used as the query type is retrieved. That query item does
/// not consist of a key-value pair because the query type is only a string.
/// Therefore, only the first item of the first retrieved pair is needed.
///
/// \param query query from which its type is used to create a corresponding
/// action
///
/// \return true if the action could be set or false if the action could not be
/// set because no corresponding query type could be found
///
bool QXmppUri::setAction(const QUrlQuery &query)
{
	// Check if there is at least one query item.
	if (query.isEmpty())
		return false;

	auto queryItems = query.queryItems();
	auto firstQueryItem = queryItems.first();

	const auto queryTypeIndex = std::find(QUERY_TYPES.cbegin(), QUERY_TYPES.cend(), firstQueryItem.first);

	// Check if the first query item is a valid action (i.e. a query item
	// with a query type as its key and without a value).
	if (queryTypeIndex == QUERY_TYPES.cend() || !firstQueryItem.second.isEmpty())
		return false;

	m_action = Action(std::distance(QUERY_TYPES.cbegin(), queryTypeIndex));
	return true;
}

///
/// Extracts the query type which represents the URI's action.
///
/// \return the query type for the URI's action
///
QString QXmppUri::queryType() const
{
	return QUERY_TYPES[int(m_action)].toString();
}

///
/// Sets the key-value pairs of a query.
///
/// \param query query which contains key-value pairs
///
void QXmppUri::setQueryKeyValuePairs(const QUrlQuery &query)
{
	switch (m_action) {
	case Message:
		m_message.setSubject(queryItemValue(query, QStringLiteral("subject")));
		m_message.setBody(queryItemValue(query, QStringLiteral("body")));
		m_message.setThread(queryItemValue(query, QStringLiteral("thread")));
		m_message.setId(queryItemValue(query, QStringLiteral("id")));
		m_message.setFrom(queryItemValue(query, QStringLiteral("from")));
		if (!queryItemValue(query, QStringLiteral("type")).isEmpty()) {
			const auto itr = std::find(MESSAGE_TYPES.cbegin(), MESSAGE_TYPES.cend(), queryItemValue(query, QStringLiteral("type")));
			if (itr != MESSAGE_TYPES.cend())
				m_message.setType(QXmppMessage::Type(std::distance(MESSAGE_TYPES.cbegin(), itr)));
		} else {
			m_hasMessageType = false;
		}
		break;
	case Login:
		m_password = queryItemValue(query, QStringLiteral("password"));
		break;
	case TrustMessage: {
		m_encryption = queryItemValue(query, QStringLiteral("encryption"));
		m_trustedKeysIds = query.allQueryItemValues(QStringLiteral("trust"), QUrl::FullyDecoded);
		m_distrustedKeysIds = query.allQueryItemValues(QStringLiteral("distrust"), QUrl::FullyDecoded);
	}
	default:
		break;
	}
}

///
/// Adds all query items of this URI to a query.
///
/// \param query query to which the items are added
///
void QXmppUri::addItemsToQuery(QUrlQuery &query) const
{
	// Add the query type for the corresponding action.
	query.addQueryItem(queryType(), {});

	// Add all remaining query items that are the key-value pairs.
	switch (m_action) {
	case Message:
		addKeyValuePairToQuery(query, QStringLiteral("from"), m_message.from());
		addKeyValuePairToQuery(query, QStringLiteral("id"), m_message.id());
		if (m_hasMessageType)
			addKeyValuePairToQuery(query, QStringLiteral("type"), MESSAGE_TYPES[int(m_message.type())]);
		addKeyValuePairToQuery(query, QStringLiteral("subject"), m_message.subject());
		addKeyValuePairToQuery(query, QStringLiteral("body"), m_message.body());
		addKeyValuePairToQuery(query, QStringLiteral("thread"), m_message.thread());
		break;
	case Login:
		addKeyValuePairToQuery(query, QStringLiteral("password"), m_password);
		break;
	case TrustMessage: {
		addKeyValuePairToQuery(query, QStringLiteral("encryption"), m_encryption);

		for (auto &identifier : m_trustedKeysIds) {
			addKeyValuePairToQuery(query, QStringLiteral("trust"), identifier);
		}

		for (auto &identifier : m_distrustedKeysIds) {
			addKeyValuePairToQuery(query, QStringLiteral("distrust"), identifier);
		}
	}
	default:
		break;
	}
}

///
/// Adds a key-value pair to a query if the value is not empty.
///
/// @param query to which the key-value pair is added
/// @param key key of the value
/// @param value value of the key
///
void QXmppUri::addKeyValuePairToQuery(QUrlQuery &query, const QString &key, QStringView value)
{
	if (!value.isEmpty())
		query.addQueryItem(key, value.toString());
}

///
/// Extracts the fully-encoded value of a query's key-value pair.
///
/// @param query query containing the key-value pair
/// @param key of the searched value
///
/// @return the value of the key
///
QString QXmppUri::queryItemValue(const QUrlQuery &query, const QString &key)
{
	return query.queryItemValue(key, QUrl::FullyDecoded);
}

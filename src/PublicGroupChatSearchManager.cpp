// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PublicGroupChatSearchManager.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QUrlQuery>

Q_LOGGING_CATEGORY(publicGroupChat_search, "public-group-chat.search", QtMsgType::QtWarningMsg)

#define THROTTLER_TIMEOUT 1000 * 60
#define NEXT_TIMEOUT 0

PublicGroupChatSearchManager::PublicGroupChatSearchManager(QNetworkAccessManager *manager, QObject *parent)
	: QObject(parent), m_throttler(new QTimer(this)), m_manager(manager)
{
	qRegisterMetaType<PublicGroupChats>();

	Q_ASSERT(m_manager);

	m_throttler->setSingleShot(true);
	m_throttler->callOnTimeout(this, &PublicGroupChatSearchManager::wakeUp);

	connect(m_manager, &QNetworkAccessManager::finished, this, &PublicGroupChatSearchManager::replyFinished);
}

PublicGroupChatSearchManager::PublicGroupChatSearchManager(QObject *parent)
	: PublicGroupChatSearchManager(new QNetworkAccessManager, parent)
{
	m_manager->setParent(this);
}

PublicGroupChatSearchManager::~PublicGroupChatSearchManager()
{
	cancel();
}

void PublicGroupChatSearchManager::requestAll()
{
	cancel();

	setIsRunning(true);

	requestFrom();
}

void PublicGroupChatSearchManager::cancel()
{
	m_throttler->stop();

	if (m_lastReply) {
		m_lastReply->abort();
	}

	m_groupChats.clear();

	setIsRunning(false);
}

bool PublicGroupChatSearchManager::isRunning() const
{
	return m_isRunning;
}

void PublicGroupChatSearchManager::setIsRunning(bool running)
{
	if (m_isRunning != running) {
		m_isRunning = running;
		Q_EMIT isRunningChanged(m_isRunning);
	}
}

PublicGroupChats PublicGroupChatSearchManager::cachedGroupChats() const
{
	return m_groupChats;
}

QNetworkRequest PublicGroupChatSearchManager::newRequest(const QString &previousAddress) const
{
	// GET /api/1.0/rooms
	// 400 - param error
	// 429 - throttled

	QUrl url(QStringLiteral("https://search.jabber.network/api/1.0/rooms"));
	QUrlQuery query;

	if (!previousAddress.isEmpty()) {
		query.addQueryItem(QStringLiteral("after"),
			QString::fromLatin1(QUrl::toPercentEncoding(previousAddress)));
	}

	url.setQuery(query);

	// The REST API does not seems to support compression and always return uncompressed data
	// so the lookup is quite expensive.
	QNetworkRequest request(url);
	request.setRawHeader(QByteArrayLiteral("Accept-Encoding"), QByteArrayLiteral("gzip, deflate"));

	qCDebug(publicGroupChat_search, "Requesting groupChats: %s", qUtf8Printable(url.toString()));

	return request;
}

void PublicGroupChatSearchManager::requestFrom(const QString &previousAddress)
{
	m_lastReply = m_manager->get(newRequest(previousAddress));
}

void PublicGroupChatSearchManager::wakeUp()
{
	requestFrom(m_groupChats.isEmpty() ? QString() : m_groupChats.constLast().address());
}

void PublicGroupChatSearchManager::replyFinished(QNetworkReply *reply)
{
	if (reply != m_lastReply) {
		// Not our reply
		return;
	}

	Q_ASSERT(m_lastReply == reply);

	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError) {
		if (reply->error() == QNetworkReply::OperationCanceledError) {
			// We did cancel the request - not an error
			qCDebug(publicGroupChat_search, "Search request aborted");
			return;
		}

		const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

		if (statusCode == 429) {
			qCDebug(publicGroupChat_search, "Search request long throttled");
			m_throttler->start(THROTTLER_TIMEOUT);
			return;
		}

		qCWarning(publicGroupChat_search, "Search request error: %s", qUtf8Printable(reply->errorString()));
		Q_EMIT error(reply->errorString());
		setIsRunning(false);
		return;
	}

	const QByteArray data = reply->readAll();
	const QJsonDocument doc = QJsonDocument::fromJson(data);
	const PublicGroupChats newGroupChats =
		PublicGroupChat::fromJson(doc.object().value(QStringLiteral("items")).toArray());

	m_groupChats.append(newGroupChats);

	if (newGroupChats.isEmpty()) {
		Q_EMIT groupChatsReceived(m_groupChats);
		setIsRunning(false);
	} else {
		qCDebug(publicGroupChat_search, "Search request fast throttled");
		m_throttler->start(NEXT_TIMEOUT);
	}
}

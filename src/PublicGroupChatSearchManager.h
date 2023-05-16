// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QLoggingCategory>
#include <QPointer>

#include "PublicGroupChat.h"

Q_DECLARE_LOGGING_CATEGORY(publicGroupChat_search)

class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;
class QTimer;

using namespace std::chrono_literals;

class PublicGroupChatSearchManager : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool isRunning READ isRunning WRITE setIsRunning NOTIFY isRunningChanged)
	Q_PROPERTY(PublicGroupChats cachedGroupChats READ cachedGroupChats NOTIFY groupChatsReceived)

public:
	static constexpr auto RequestTimeout = 60s;

	explicit PublicGroupChatSearchManager(QNetworkAccessManager *manager, QObject *parent = nullptr);
	explicit PublicGroupChatSearchManager(QObject *parent = nullptr);
	~PublicGroupChatSearchManager() override;

	bool isRunning() const;
	PublicGroupChats cachedGroupChats() const;

	Q_SLOT void requestAll();
	Q_SLOT void cancel();

	Q_SIGNAL void isRunningChanged(bool running);
	Q_SIGNAL void error(const QString &error);
	Q_SIGNAL void groupChatsReceived(const PublicGroupChats &groupChats);

private:
	void setIsRunning(bool running);
	QNetworkRequest newRequest(const QString &previousAddress = {}) const;
	void requestFrom(const QString &previousAddress = {});
	void wakeUp();
	void replyFinished(QNetworkReply *reply);
	QString saveFilePath() const;
	bool saveGroupChats();
	bool readGroupChats();

	QTimer *m_throttler = nullptr;
	QNetworkAccessManager *m_manager = nullptr;
	QPointer<QNetworkReply> m_lastReply;
	bool m_isRunning = false;
	PublicGroupChats m_groupChats;
};

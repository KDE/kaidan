// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PublicGroupChatSearchController.h"

// Qt
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUrlQuery>
// Kaidan
#include "KaidanCoreLog.h"

#define NEXT_TIMEOUT 0

PublicGroupChatSearchController::PublicGroupChatSearchController(QNetworkAccessManager *manager, QObject *parent)
    : QObject(parent)
    , m_throttler(new QTimer(this))
    , m_manager(manager)
{
    qRegisterMetaType<PublicGroupChats>();

    Q_ASSERT(m_manager);

    m_throttler->setSingleShot(true);
    m_throttler->callOnTimeout(this, &PublicGroupChatSearchController::wakeUp);

    connect(m_manager, &QNetworkAccessManager::finished, this, &PublicGroupChatSearchController::replyFinished);
}

PublicGroupChatSearchController::PublicGroupChatSearchController(QObject *parent)
    : PublicGroupChatSearchController(new QNetworkAccessManager, parent)
{
    m_manager->setParent(this);
}

PublicGroupChatSearchController::~PublicGroupChatSearchController()
{
    cancel();
}

void PublicGroupChatSearchController::requestAll()
{
    cancel();

    setIsRunning(true);

    requestFrom();
}

void PublicGroupChatSearchController::cancel()
{
    m_throttler->stop();

    if (m_lastReply) {
        m_lastReply->abort();
    }

    m_groupChats.clear();

    setIsRunning(false);
}

bool PublicGroupChatSearchController::isRunning() const
{
    return m_isRunning;
}

void PublicGroupChatSearchController::setIsRunning(bool running)
{
    if (m_isRunning != running) {
        m_isRunning = running;
        Q_EMIT isRunningChanged(m_isRunning);
    }
}

PublicGroupChats PublicGroupChatSearchController::cachedGroupChats() const
{
    return m_groupChats;
}

QNetworkRequest PublicGroupChatSearchController::newRequest(const QString &previousAddress) const
{
    // GET /api/1.0/rooms
    // 400 - param error
    // 429 - throttled

    QUrl url(QStringLiteral("https://search.jabber.network/api/1.0/rooms"));
    QUrlQuery query;

    if (!previousAddress.isEmpty()) {
        query.addQueryItem(QStringLiteral("after"), QString::fromLatin1(QUrl::toPercentEncoding(previousAddress)));
    }

    url.setQuery(query);

    // The REST API does not seems to support compression and always return uncompressed data
    // so the lookup is quite expensive.
    QNetworkRequest request(url);
    request.setRawHeader(QByteArrayLiteral("Accept-Encoding"), QByteArrayLiteral("gzip, deflate"));

    qCDebug(KAIDAN_CORE_LOG, "Requesting groupChats: %s", qUtf8Printable(url.toString()));

    return request;
}

void PublicGroupChatSearchController::requestFrom(const QString &previousAddress)
{
    m_lastReply = m_manager->get(newRequest(previousAddress));
}

void PublicGroupChatSearchController::wakeUp()
{
    requestFrom(m_groupChats.isEmpty() ? QString() : m_groupChats.constLast().address());
}

void PublicGroupChatSearchController::replyFinished(QNetworkReply *reply)
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
            qCDebug(KAIDAN_CORE_LOG, "Search request aborted");
            return;
        }

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode == 429) {
            qCDebug(KAIDAN_CORE_LOG, "Search request long throttled");
            m_throttler->start(RequestTimeout);
            return;
        }

        qCWarning(KAIDAN_CORE_LOG, "Search request error: %s", qUtf8Printable(reply->errorString()));

        if (readGroupChats()) {
            Q_EMIT groupChatsReceived(m_groupChats);
        } else {
            Q_EMIT error(reply->errorString());
        }

        setIsRunning(false);
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    const PublicGroupChats newGroupChats = PublicGroupChat::fromJson(doc.object().value(QStringLiteral("items")).toArray());

    m_groupChats.append(newGroupChats);

    if (newGroupChats.isEmpty()) {
        saveGroupChats();
        Q_EMIT groupChatsReceived(m_groupChats);
        setIsRunning(false);
    } else {
        qCDebug(KAIDAN_CORE_LOG, "Search request fast throttled");
        m_throttler->start(NEXT_TIMEOUT);
    }
}

QString PublicGroupChatSearchController::saveFilePath() const
{
    // Don't bother to do checks, Database already do them.
    const auto appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const auto dir = QDir(appDataPath);

    if (!dir.mkpath(QStringLiteral("."))) {
        qCWarning(KAIDAN_CORE_LOG, "Cannot create dir: %ls", qUtf16Printable(dir.absolutePath()));
        return {};
    }

    return dir.absoluteFilePath(QStringLiteral("public-group-chats.json"));
}

bool PublicGroupChatSearchController::saveGroupChats()
{
    QSaveFile file(saveFilePath());

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(KAIDAN_CORE_LOG, "Cannot open file for writing: %ls, %ls", qUtf16Printable(file.fileName()), qUtf16Printable(file.errorString()));
        file.cancelWriting();
        return false;
    }

    const auto json = QJsonDocument(PublicGroupChat::toJson(m_groupChats)).toJson();

    if (file.write(json) == -1) {
        qCWarning(KAIDAN_CORE_LOG, "Cannot save public group chats: %ls, %ls", qUtf16Printable(file.fileName()), qUtf16Printable(file.errorString()));
        file.cancelWriting();
        return false;
    }

    return file.commit();
}

bool PublicGroupChatSearchController::readGroupChats()
{
    const auto filePath = saveFilePath();

    if (!QFile::exists(filePath)) {
        return false;
    }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(KAIDAN_CORE_LOG, "Cannot open file for reading: %ls, %ls", qUtf16Printable(file.fileName()), qUtf16Printable(file.errorString()));
        return false;
    }

    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);

    if (error.error != QJsonParseError::NoError) {
        qCWarning(KAIDAN_CORE_LOG, "Cannot parse json file: %ls, %ls", qUtf16Printable(file.fileName()), qUtf16Printable(error.errorString()));
        return false;
    }

    m_groupChats = PublicGroupChat::fromJson(document.array());
    return true;
}

#include "moc_PublicGroupChatSearchController.cpp"

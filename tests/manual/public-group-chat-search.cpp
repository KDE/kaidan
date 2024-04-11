// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

template<typename T>
T keyValue(const QJsonObject &object, const QStringView &key);

template<>
QString keyValue(const QJsonObject &object, const QStringView &key)
{
    return object.value(key).toString();
}

template<>
int keyValue(const QJsonObject &object, const QStringView &key)
{
    return static_cast<int>(object.value(key).toDouble());
}

template<>
bool keyValue(const QJsonObject &object, const QStringView &key)
{
    return object.value(key).toBool();
}

using PublicGroupChats = QVector<struct PublicGroupChat>;

struct PublicGroupChat {
    static constexpr const QStringView Address = u"address";
    static constexpr const QStringView Users = u"nusers";
    static constexpr const QStringView IsOpen = u"is_open";
    static constexpr const QStringView Name = u"name";
    static constexpr const QStringView Description = u"description";
    static constexpr const QStringView Language = u"language";

    explicit PublicGroupChat(const QJsonObject &object)
    {
        Q_ASSERT(!object.isEmpty());

        address = keyValue<QString>(object, Address);
        nusers = keyValue<int>(object, Users);
        is_open = keyValue<bool>(object, IsOpen);
        name = keyValue<QString>(object, Name);
        description = keyValue<QString>(object, Description);
        language = keyValue<QString>(object, Language);
    }

    QJsonObject toJson() const
    {
        return {
            {Address.toString(), address},
            {Users.toString(), nusers},
            {IsOpen.toString(), is_open},
            {Name.toString(), name},
            {Description.toString(), description},
            {Language.toString(), language},
        };
    }

    static PublicGroupChats fromJson(const QJsonArray &array)
    {
        PublicGroupChats groups;

        groups.reserve(array.count());

        for (const auto &value : array) {
            groups.append(PublicGroupChat{value.toObject()});
        }

        return groups;
    }

    static QJsonArray toJson(const PublicGroupChats &groups)
    {
        QJsonArray array;

        for (const auto &group : groups) {
            array.append(group.toJson());
        }

        return array;
    }

    QString address;
    int nusers;
    bool is_open;
    QString name;
    QString description;
    QString language;
};

PublicGroupChats listEntries(const QLocale &locale, QNetworkAccessManager &nm, const QString &previous)
{
    // GET /api/1.0/rooms
    // 400 - param error
    // 429 - throttled

    qInfo() << "Requesting entries with previous (" << previous << ")...";

    static qint64 total = 0;

    QUrl url(QStringLiteral("https://search.jabber.network/api/1.0/rooms"));
    QUrlQuery query;

    if (!previous.isEmpty()) {
        query.addQueryItem(QStringLiteral("after"), previous);
    }

    url.setQuery(query);

    // The REST API does not seems to support compression and always return uncompressed data
    // so the lookup is quite expensive.
    QNetworkRequest request(url);
    request.setRawHeader(QByteArrayLiteral("Accept-Encoding"), QByteArrayLiteral("gzip, deflate"));

    QEventLoop loop;
    auto reply = nm.get(request);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    loop.exec();

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode == 429) {
            // Throttled for 1min
            PublicGroupChats groups;

            qInfo() << "Throttled for 1min...";

            QTimer::singleShot(1000 * 60, &nm, [&groups, &locale, &nm, &previous, &loop]() {
                groups = listEntries(locale, nm, previous);
                loop.quit();
            });

            loop.exec();
            return groups;
        }

        qWarning() << "Error:" << request.url() << reply->errorString();
        return {};
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);

    total += data.size();

    qDebug() << "Downloaded so far:" << locale.formattedDataSize(total);

    return PublicGroupChat::fromJson(doc.object().value(QStringLiteral("items")).toArray());
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QLocale locale = QLocale::system();
    QNetworkAccessManager nm;
    PublicGroupChats all;
    PublicGroupChats groups;

    while (!(groups = listEntries(locale, nm, all.isEmpty() ? QString() : all.constLast().address)).isEmpty()) {
        all.append(groups);
        QThread::msleep(250);
    }

    const QByteArray data = QJsonDocument(PublicGroupChat::toJson(all)).toJson();

    qWarning() << "Got" << all.count() << "entries occupying" << locale.formattedDataSize(data.size()) << "uncompressed bytes,"
               << locale.formattedDataSize(qCompress(data).size()) << "compressed.";

    return 0;
}

// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QSignalSpy>
#include <QTest>

#include <QXmppUtils.h>

#include "PresenceCache.h"
#include "Test.h"

Q_DECLARE_METATYPE(QList<QXmppPresence>)

class PresenceCacheTest : public Test
{
    Q_OBJECT

private:
    Q_SLOT void initTestCase() override;
    Q_SLOT void presenceChangedSignal();
    Q_SLOT void presenceGetter_data();
    Q_SLOT void presenceGetter();
    Q_SLOT void idealResource_data();
    Q_SLOT void idealResource();

    void addBasicPresences();
    void addSimplePresence(const QString &jid,
                           QXmppPresence::AvailableStatusType available = QXmppPresence::Online,
                           const QString &status = {},
                           QXmppPresence::Type type = QXmppPresence::Available);
    QXmppPresence simplePresence(const QString &jid,
                                 QXmppPresence::AvailableStatusType available = QXmppPresence::Online,
                                 const QString &status = {},
                                 QXmppPresence::Type type = QXmppPresence::Available);

    PresenceCache cache;
};

void PresenceCacheTest::initTestCase()
{
    Test::initTestCase();

    qRegisterMetaType<PresenceCache::ChangeType>();
}

void PresenceCacheTest::presenceChangedSignal()
{
    cache.clear();

    enum {
        ChangeType,
        Jid,
        Resource,
    };

    QXmppPresence p1;
    p1.setFrom(QStringLiteral("bob@kaidan.im/dev1"));
    QXmppPresence p2(QXmppPresence::Unavailable);
    p2.setFrom(QStringLiteral("bob@kaidan.im/dev1"));
    QXmppPresence p3(QXmppPresence::Unavailable);
    p3.setFrom(QStringLiteral("bob@kaidan.im/dev23"));

    QSignalSpy spy(&cache, &PresenceCache::presenceChanged);
    auto signalCount = 0;

    QVERIFY(spy.isValid());
    QVERIFY(spy.isEmpty());

    cache.updatePresence(p1);
    QCOMPARE(spy.count(), ++signalCount);
    QCOMPARE(spy[0][ChangeType], QVariant::fromValue(PresenceCache::Connected));
    QCOMPARE(spy[0][Jid], QStringLiteral("bob@kaidan.im"));
    QCOMPARE(spy[0][Resource], QStringLiteral("dev1"));

    cache.updatePresence(p1);
    QCOMPARE(spy.count(), ++signalCount);
    QCOMPARE(spy[1][ChangeType], QVariant::fromValue(PresenceCache::Updated));

    cache.updatePresence(p1);
    QCOMPARE(spy.count(), ++signalCount);
    QCOMPARE(spy[2][ChangeType], QVariant::fromValue(PresenceCache::Updated));

    cache.updatePresence(p2);
    QCOMPARE(spy.count(), ++signalCount);
    QCOMPARE(spy[3][ChangeType], QVariant::fromValue(PresenceCache::Disconnected));

    // test unknown device disconnect
    cache.updatePresence(p3);
    QCOMPARE(spy.count(), signalCount);
}

void PresenceCacheTest::presenceGetter_data()
{
    QTest::addColumn<QString>("jid");
    QTest::addColumn<QString>("resource");
    QTest::addColumn<bool>("exists");

    QTest::newRow("bob1") << QStringLiteral("bob@kaidan.im") << QStringLiteral("dev1") << true;
    QTest::newRow("bob2") << QStringLiteral("bob@kaidan.im") << QStringLiteral("dev2") << true;
    QTest::newRow("alice1") << QStringLiteral("alice@kaidan.im") << QStringLiteral("kdn1") << true;
    QTest::newRow("alice2") << QStringLiteral("alice@kaidan.im") << QStringLiteral("kdn2") << true;
    QTest::newRow("unknwon presence") << QStringLiteral("alice@kaidan.im") << QStringLiteral("device3") << false;
}

void PresenceCacheTest::presenceGetter()
{
    QFETCH(QString, jid);
    QFETCH(QString, resource);
    QFETCH(bool, exists);

    cache.clear();
    addBasicPresences();

    auto presence = cache.presence(jid, resource);
    QCOMPARE(presence.has_value(), exists);
    if (exists) {
        QCOMPARE(presence->from(), QStringView(u"%1/%2").arg(jid, resource));
    }
}

void PresenceCacheTest::idealResource_data()
{
    QTest::addColumn<QList<QXmppPresence>>("presences");
    QTest::addColumn<QString>("expectedResource");

#define ROW(name, presences, expectedResource) QTest::newRow(name) << (presences) << QStringLiteral(expectedResource)

    ROW("DND",
        QList<QXmppPresence>() << simplePresence(QStringLiteral("a@b/1"), QXmppPresence::Away) << simplePresence(QStringLiteral("a@b/2"), QXmppPresence::XA)
                               << simplePresence(QStringLiteral("a@b/3"), QXmppPresence::Online) << simplePresence(QStringLiteral("a@b/4"), QXmppPresence::Chat)
                               << simplePresence(QStringLiteral("a@b/5"), QXmppPresence::DND),
        "5");
    ROW("Chat",
        QList<QXmppPresence>() << simplePresence(QStringLiteral("a@b/1"), QXmppPresence::Away) << simplePresence(QStringLiteral("a@b/2"), QXmppPresence::XA)
                               << simplePresence(QStringLiteral("a@b/3"), QXmppPresence::Chat)
                               << simplePresence(QStringLiteral("a@b/4"), QXmppPresence::Online),
        "3");
    ROW("XA",
        QList<QXmppPresence>() << simplePresence(QStringLiteral("a@b/1"), QXmppPresence::XA) << simplePresence(QStringLiteral("a@b/2"), QXmppPresence::Away),
        "1");
    ROW("Away",
        QList<QXmppPresence>() << simplePresence(QStringLiteral("a@b/1"), QXmppPresence::Away) << simplePresence(QStringLiteral("a@b/2"), QXmppPresence::XA),
        "1");
    ROW("status text [DND]",
        QList<QXmppPresence>() << simplePresence(QStringLiteral("a@b/1"), QXmppPresence::Away) << simplePresence(QStringLiteral("a@b/2"), QXmppPresence::XA)
                               << simplePresence(QStringLiteral("a@b/3"), QXmppPresence::Online) << simplePresence(QStringLiteral("a@b/4"), QXmppPresence::Chat)
                               << simplePresence(QStringLiteral("a@b/5"), QXmppPresence::DND)
                               << simplePresence(QStringLiteral("a@b/6"), QXmppPresence::Chat, QStringLiteral("my status"))
                               << simplePresence(QStringLiteral("a@b/7"), QXmppPresence::DND, QStringLiteral("my status"))
                               << simplePresence(QStringLiteral("a@b/8"), QXmppPresence::Away, QStringLiteral("my status")),
        "7");
    ROW("status text [XA]",
        QList<QXmppPresence>() << simplePresence(QStringLiteral("a@b/1"), QXmppPresence::Away) << simplePresence(QStringLiteral("a@b/2"), QXmppPresence::XA)
                               << simplePresence(QStringLiteral("a@b/3"), QXmppPresence::XA, QStringLiteral("my status"))
                               << simplePresence(QStringLiteral("a@b/4"), QXmppPresence::Away, QStringLiteral("my status")),
        "3");

#undef ROW
}

void PresenceCacheTest::idealResource()
{
    QFETCH(QList<QXmppPresence>, presences);
    QFETCH(QString, expectedResource);

    cache.clear();
    for (const auto &presence : std::as_const(presences))
        cache.updatePresence(presence);

    const auto jid = QXmppUtils::jidToBareJid(presences.first().from());
    QCOMPARE(cache.pickIdealResource(jid), expectedResource);
}

void PresenceCacheTest::addBasicPresences()
{
    addSimplePresence(QStringLiteral("bob@kaidan.im/dev1"));
    addSimplePresence(QStringLiteral("bob@kaidan.im/dev2"));
    addSimplePresence(QStringLiteral("alice@kaidan.im/kdn1"));
    addSimplePresence(QStringLiteral("alice@kaidan.im/kdn2"));
}

void PresenceCacheTest::addSimplePresence(const QString &jid, QXmppPresence::AvailableStatusType available, const QString &status, QXmppPresence::Type type)
{
    cache.updatePresence(simplePresence(jid, available, status, type));
}

QXmppPresence
PresenceCacheTest::simplePresence(const QString &jid, QXmppPresence::AvailableStatusType available, const QString &status, QXmppPresence::Type type)
{
    QXmppPresence p(type);
    p.setFrom(jid);
    p.setStatusText(status);
    p.setAvailableStatusType(available);
    return p;
}

QTEST_GUILESS_MAIN(PresenceCacheTest)
#include "PresenceCacheTest.moc"

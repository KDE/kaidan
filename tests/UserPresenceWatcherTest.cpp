// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>

#include <QXmppUtils.h>

#include "../src/PresenceCache.h"

Q_DECLARE_METATYPE(QXmppPresence::AvailableStatusType)

class UserPresenceWatcherTest : public QObject
{
    Q_OBJECT

private:
    Q_SLOT void initTestCase();
    Q_SLOT void testBasic();

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

void UserPresenceWatcherTest::initTestCase()
{
    qRegisterMetaType<PresenceCache::ChangeType>();
}

void UserPresenceWatcherTest::testBasic()
{
    cache.clear();

    // set user model JID
    UserPresenceWatcher userModel;
    userModel.setJid(QStringLiteral("bob@kaidan.im"));

    QSignalSpy resourceSpy(&userModel, &UserPresenceWatcher::resourceChanged);
    QSignalSpy presencePropertiesSpy(&userModel, &UserPresenceWatcher::presencePropertiesChanged);

    // add presences for bob@kaidan.im
    // the user model needs to handle the new presences
    addBasicPresences();

    QCOMPARE(resourceSpy.count(), 1);
    QCOMPARE(presencePropertiesSpy.count(), 1);

    QCOMPARE(userModel.resource(), QStringLiteral("dev1"));
    QCOMPARE(userModel.availability(), Presence::Online);
    QCOMPARE(userModel.statusText(), QString());

    // add more important presence
    addSimplePresence(QStringLiteral("bob@kaidan.im/mobile"), QXmppPresence::Online, QStringLiteral("I like cats."));

    QCOMPARE(resourceSpy.count(), 2);
    QCOMPARE(presencePropertiesSpy.count(), 2);

    QCOMPARE(userModel.resource(), QStringLiteral("mobile"));
    QCOMPARE(userModel.availability(), Presence::Online);
    QCOMPARE(userModel.statusText(), QStringLiteral("I like cats."));

    // change presence
    addSimplePresence(QStringLiteral("bob@kaidan.im/mobile"), QXmppPresence::DND, QStringLiteral("At work"));

    QCOMPARE(resourceSpy.count(), 2);
    QCOMPARE(presencePropertiesSpy.count(), 3);

    QCOMPARE(userModel.resource(), QStringLiteral("mobile"));
    QCOMPARE(userModel.availability(), Presence::DND);
    QCOMPARE(userModel.statusText(), QStringLiteral("At work"));

    // device goes offline
    addSimplePresence(QStringLiteral("bob@kaidan.im/mobile"),
                      QXmppPresence::Online,
                      QStringLiteral("No user interaction since 5 minutes."),
                      QXmppPresence::Unavailable);

    QCOMPARE(resourceSpy.count(), 3);
    QCOMPARE(presencePropertiesSpy.count(), 4);

    QCOMPARE(userModel.resource(), QStringLiteral("dev1"));
    QCOMPARE(userModel.availability(), Presence::Online);
    QCOMPARE(userModel.statusText(), QString());

    // reset presences
    cache.clear();

    QCOMPARE(resourceSpy.count(), 4);
    QCOMPARE(presencePropertiesSpy.count(), 5);

    QCOMPARE(userModel.resource(), QString());
    QCOMPARE(userModel.availability(), Presence::Offline);
    QCOMPARE(userModel.statusText(), QString());
}

void UserPresenceWatcherTest::addBasicPresences()
{
    addSimplePresence(QStringLiteral("bob@kaidan.im/dev1"));
    addSimplePresence(QStringLiteral("bob@kaidan.im/dev2"));
    addSimplePresence(QStringLiteral("alice@kaidan.im/kdn1"));
    addSimplePresence(QStringLiteral("alice@kaidan.im/kdn2"));
}

void UserPresenceWatcherTest::addSimplePresence(const QString &jid,
                                                QXmppPresence::AvailableStatusType available,
                                                const QString &status,
                                                QXmppPresence::Type type)
{
    cache.updatePresence(simplePresence(jid, available, status, type));
}

QXmppPresence
UserPresenceWatcherTest::simplePresence(const QString &jid, QXmppPresence::AvailableStatusType available, const QString &status, QXmppPresence::Type type)
{
    QXmppPresence p(type);
    p.setFrom(jid);
    p.setStatusText(status);
    p.setAvailableStatusType(available);
    return p;
}

QTEST_GUILESS_MAIN(UserPresenceWatcherTest)
#include "UserPresenceWatcherTest.moc"

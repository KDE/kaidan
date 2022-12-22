// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "../src/PublicGroupChat.h"
#include "../src/PublicGroupChatModel.h"
#include "../src/PublicGroupChatSearchManager.h"

class GroupChatTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void test_splitLanguages_data()
	{
		QTest::addColumn<QString>("languages");
		QTest::addColumn<QStringList>("expected");

		QTest::newRow(",ko,, | en , 'fr' ,") << QStringLiteral(",ko,, | en , 'fr' ,")
						     << QStringList {
								QStringLiteral("en"),
								QStringLiteral("fr"),
								QStringLiteral("ko"),
							};
		QTest::newRow("z t c m k l a") << QStringLiteral("z t c m k l a")
					       << QStringList {
							  QStringLiteral("a"),
							  QStringLiteral("c"),
							  QStringLiteral("k"),
							  QStringLiteral("l"),
							  QStringLiteral("m"),
							  QStringLiteral("t"),
							  QStringLiteral("z"),
						  };
		QTest::newRow("'en',fr,'ko'") << QStringLiteral("'en',fr,'ko'")
					      << QStringList {
							 QStringLiteral("en"),
							 QStringLiteral("fr"),
							 QStringLiteral("ko"),
						 };
		QTest::newRow("bbb aaa") << QStringLiteral("bbb aaa")
					 << QStringList {
						    QStringLiteral("aaa"),
						    QStringLiteral("bbb"),
					    };
		QTest::newRow("ko, en | fr , ") << QStringLiteral("ko, en | fr , ")
						<< QStringList {
							   QStringLiteral("en"),
							   QStringLiteral("fr"),
							   QStringLiteral("ko"),
						   };
	}

	void test_splitLanguages()
	{
		QFETCH(QString, languages);
		QFETCH(QStringList, expected);

		QCOMPARE(PublicGroupChat::splitLanguages(languages), expected);
	}

	void test_GroupChat()
	{
		PublicGroupChat groupChat;

		groupChat.setAddress(QStringLiteral("address@jabber.com"));
		groupChat.setUsers(42);
		groupChat.setIsOpen(false);
		groupChat.setName(QStringLiteral("Kaidan Group Chat"));
		groupChat.setLanguages({QStringLiteral("en")});

		const QJsonObject object = QJsonObject {
			{PublicGroupChat::Address.toString(), QStringLiteral("address@jabber.com")},
			{PublicGroupChat::Users.toString(), 42},
			{PublicGroupChat::IsOpen.toString(), false},
			{PublicGroupChat::Name.toString(), QStringLiteral("Kaidan Group Chat")},
			{PublicGroupChat::Language.toString(), QStringLiteral("en")},
		};

		QVERIFY(groupChat.toJson() == object);
		QVERIFY(groupChat == PublicGroupChat(object));
	}

	void test_GroupChats()
	{
		PublicGroupChat groupChat1;

		groupChat1.setAddress(QStringLiteral("address.en@jabber.com"));
		groupChat1.setUsers(42);
		groupChat1.setIsOpen(false);
		groupChat1.setName(QStringLiteral("Kaidan Group Chat English"));
		groupChat1.setLanguages({QStringLiteral("en")});

		PublicGroupChat groupChat2;

		groupChat2.setAddress(QStringLiteral("address.fr@jabber.com"));
		groupChat2.setUsers(42);
		groupChat2.setIsOpen(false);
		groupChat2.setName(QStringLiteral("Kaidan Group Chat French"));
		groupChat2.setLanguages({QStringLiteral("fr")});

		const PublicGroupChats groupChats {
			groupChat1,
			groupChat2,
		};
		const QJsonObject object1 = QJsonObject {
			{PublicGroupChat::Address.toString(), QStringLiteral("address.en@jabber.com")},
			{PublicGroupChat::Users.toString(), 42},
			{PublicGroupChat::IsOpen.toString(), false},
			{PublicGroupChat::Name.toString(), QStringLiteral("Kaidan Group Chat English")},
			{PublicGroupChat::Language.toString(), QStringLiteral("en")},
		};
		const QJsonObject object2 = QJsonObject {
			{PublicGroupChat::Address.toString(), QStringLiteral("address.fr@jabber.com")},
			{PublicGroupChat::Users.toString(), 42},
			{PublicGroupChat::IsOpen.toString(), false},
			{PublicGroupChat::Name.toString(), QStringLiteral("Kaidan Group Chat French")},
			{PublicGroupChat::Language.toString(), QStringLiteral("fr")},
		};

		QVERIFY(groupChat1 != groupChat2);
		QVERIFY(PublicGroupChat::toJson(groupChats) == QJsonArray({object1, object2}));
		QVERIFY(groupChats == (PublicGroupChats {PublicGroupChat {object1}, PublicGroupChat {object2}}));
	}

	void test_GroupChatSearchManager_GroupChatModel()
	{
		PublicGroupChatSearchManager manager;
		PublicGroupChatModel model;
		QSignalSpy spyIsRunning(&manager, &PublicGroupChatSearchManager::isRunningChanged);
		QSignalSpy spyError(&manager, &PublicGroupChatSearchManager::error);
		QSignalSpy spyReceived(&manager, &PublicGroupChatSearchManager::groupChatsReceived);
		auto clearSpies = [&]() {
			spyIsRunning.clear();
			spyError.clear();
			spyReceived.clear();
		};

		connect(&manager, &PublicGroupChatSearchManager::groupChatsReceived, &model, &PublicGroupChatModel::setGroupChats);

		QVERIFY(manager.cachedGroupChats().isEmpty());
		QVERIFY(model.rowCount() == manager.cachedGroupChats().count());

		// requestAll and cancel after 1 second
		manager.requestAll();
		QTimer::singleShot(1000, &manager, &PublicGroupChatSearchManager::cancel);

		QVERIFY(spyIsRunning.wait(2000));
		QVERIFY(spyError.isEmpty());
		QVERIFY(spyReceived.isEmpty());
		QCOMPARE(spyIsRunning.count(), 2);
		QCOMPARE(spyIsRunning.constFirst().constFirst().toBool(), true);
		QCOMPARE(spyIsRunning.constLast().constFirst().toBool(), false);
		QVERIFY(manager.cachedGroupChats().isEmpty());
		QVERIFY(model.rowCount() == manager.cachedGroupChats().count());

		clearSpies();

		// Really requestAll
		manager.requestAll();

		QVERIFY(spyIsRunning.wait(30000));
		QVERIFY(spyError.isEmpty());
		QCOMPARE(spyReceived.count(), 1);
		QCOMPARE(spyIsRunning.count(), 2);
		QCOMPARE(spyIsRunning.constFirst().constFirst().toBool(), true);
		QCOMPARE(spyIsRunning.constLast().constFirst().toBool(), false);
		QVERIFY(!manager.cachedGroupChats().isEmpty());
		QVERIFY(model.rowCount() == manager.cachedGroupChats().count());

		// Check first group chat
		const PublicGroupChat &groupChat = model.groupChats().constFirst();
		const QModelIndex index = model.index(0);

		QVERIFY(!groupChat.name().isEmpty());
		QCOMPARE(groupChat.name(), index.data(Qt::DisplayRole));
		QCOMPARE(groupChat.description(), index.data(Qt::ToolTipRole));
	}
};

QTEST_GUILESS_MAIN(GroupChatTest)
#include "PublicGroupChatTest.moc"

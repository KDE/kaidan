// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "../src/PublicGroupChat.h"
#include "../src/PublicGroupChatModel.h"
#include "../src/PublicGroupChatProxyModel.h"
#include "../src/PublicGroupChatSearchManager.h"

#define REQUEST_TIMEOUT \
	static_cast<int>( \
		(std::chrono::milliseconds(PublicGroupChatSearchManager::RequestTimeout) * 1.2).count())

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

		QVERIFY(spyIsRunning.wait(REQUEST_TIMEOUT));
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

	void test_GroupChatProxyModel_data()
	{
		using Role = PublicGroupChatModel::CustomRole;
		QTest::addColumn<Role>("sortRole");
		QTest::addColumn<Role>("filterRole");
		QTest::addColumn<QString>("filterWildcard");
		QTest::addColumn<QVector<QStringList>>("expected"); // Names, Addresses, Languages

		//

		QTest::newRow("Name / Name / Null")
			<< Role::Name << Role::Name << QString()
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("Bookri"),
						   QStringLiteral("PasNox"),
						   QStringLiteral("ZyNox"),
					   },
					   QStringList {
						   QStringLiteral("bookri@jabber.com"),
						   QStringLiteral("pasnox@jabber.com"),
						   QStringLiteral("zynox@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("fr"),
						   QStringLiteral("en"),
						   QStringLiteral("de"),
					   },
				   },
			   };

		QTest::newRow("Address / Name / Null")
			<< Role::Address << Role::Name << QString()
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("Bookri"),
						   QStringLiteral("PasNox"),
						   QStringLiteral("ZyNox"),
					   },
					   QStringList {
						   QStringLiteral("bookri@jabber.com"),
						   QStringLiteral("pasnox@jabber.com"),
						   QStringLiteral("zynox@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("fr"),
						   QStringLiteral("en"),
						   QStringLiteral("de"),
					   },
				   },
			   };

		QTest::newRow("Languages / Name / Null")
			<< Role::Languages << Role::Name << QString()
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("ZyNox"),
						   QStringLiteral("PasNox"),
						   QStringLiteral("Bookri"),
					   },
					   QStringList {
						   QStringLiteral("zynox@jabber.com"),
						   QStringLiteral("pasnox@jabber.com"),
						   QStringLiteral("bookri@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("de"),
						   QStringLiteral("en"),
						   QStringLiteral("fr"),
					   },
				   },
			   };

		//

		QTest::newRow("Name / Name / nox")
			<< Role::Name << Role::Name << QStringLiteral("nox")
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("PasNox"),
						   QStringLiteral("ZyNox"),
					   },
					   QStringList {
						   QStringLiteral("pasnox@jabber.com"),
						   QStringLiteral("zynox@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("en"),
						   QStringLiteral("de"),
					   },
				   },
			   };

		QTest::newRow("Address / Name / nox")
			<< Role::Address << Role::Name << QStringLiteral("nox")
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("PasNox"),
						   QStringLiteral("ZyNox"),
					   },
					   QStringList {
						   QStringLiteral("pasnox@jabber.com"),
						   QStringLiteral("zynox@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("en"),
						   QStringLiteral("de"),
					   },
				   },
			   };

		QTest::newRow("Languages / Name / nox")
			<< Role::Languages << Role::Name << QStringLiteral("nox")
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("ZyNox"),
						   QStringLiteral("PasNox"),
					   },
					   QStringList {
						   QStringLiteral("zynox@jabber.com"),
						   QStringLiteral("pasnox@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("de"),
						   QStringLiteral("en"),
					   },
				   },
			   };

		//

		QTest::newRow("Name / Address / nox")
			<< Role::Name << Role::Address << QStringLiteral("nox")
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("PasNox"),
						   QStringLiteral("ZyNox"),
					   },
					   QStringList {
						   QStringLiteral("pasnox@jabber.com"),
						   QStringLiteral("zynox@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("en"),
						   QStringLiteral("de"),
					   },
				   },
			   };

		QTest::newRow("Address / Address / nox")
			<< Role::Address << Role::Address << QStringLiteral("nox")
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("PasNox"),
						   QStringLiteral("ZyNox"),
					   },
					   QStringList {
						   QStringLiteral("pasnox@jabber.com"),
						   QStringLiteral("zynox@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("en"),
						   QStringLiteral("de"),
					   },
				   },
			   };

		QTest::newRow("Languages / Address / nox")
			<< Role::Languages << Role::Address << QStringLiteral("nox")
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("ZyNox"),
						   QStringLiteral("PasNox"),
					   },
					   QStringList {
						   QStringLiteral("zynox@jabber.com"),
						   QStringLiteral("pasnox@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("de"),
						   QStringLiteral("en"),
					   },
				   },
			   };

		//

		QTest::newRow("Name / Languages / nox")
			<< Role::Name << Role::Languages << QStringLiteral("nox")
			<< QVector<QStringList> {
				   {
					   QStringList {},
					   QStringList {},
					   QStringList {},
				   },
			   };

		QTest::newRow("Address / Languages / es")
			<< Role::Address << Role::Languages << QStringLiteral("es")
			<< QVector<QStringList> {
				   {
					   QStringList {},
					   QStringList {},
					   QStringList {},
				   },
			   };

		QTest::newRow("Languages / Languages / fr")
			<< Role::Languages << Role::Languages << QStringLiteral("fr")
			<< QVector<QStringList> {
				   {
					   QStringList {"Bookri"},
					   QStringList {"bookri@jabber.com"},
					   QStringList {"fr"},
				   },
			   };

		//

		QTest::newRow("Name / GlobalSearch / jabber")
			<< Role::Name << Role::GlobalSearch << QStringLiteral("jabber")
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("Bookri"),
						   QStringLiteral("PasNox"),
						   QStringLiteral("ZyNox"),
					   },
					   QStringList {
						   QStringLiteral("bookri@jabber.com"),
						   QStringLiteral("pasnox@jabber.com"),
						   QStringLiteral("zynox@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("fr"),
						   QStringLiteral("en"),
						   QStringLiteral("de"),
					   },
				   },
			   };

		QTest::newRow("Languages / GlobalSearch / jabber")
			<< Role::Languages << Role::GlobalSearch << QStringLiteral("jabber")
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("ZyNox"),
						   QStringLiteral("PasNox"),
						   QStringLiteral("Bookri"),
					   },
					   QStringList {
						   QStringLiteral("zynox@jabber.com"),
						   QStringLiteral("pasnox@jabber.com"),
						   QStringLiteral("bookri@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("de"),
						   QStringLiteral("en"),
						   QStringLiteral("fr"),
					   },
				   },
			   };

		QTest::newRow("Languages / GlobalSearch / kri")
			<< Role::Languages << Role::GlobalSearch << QStringLiteral("kri")
			<< QVector<QStringList> {
				   {
					   QStringList {
						   QStringLiteral("Bookri"),
					   },
					   QStringList {
						   QStringLiteral("bookri@jabber.com"),
					   },
					   QStringList {
						   QStringLiteral("fr"),
					   },
				   },
			   };
	}

	void test_GroupChatProxyModel()
	{
		using Role = PublicGroupChatModel::CustomRole;
		PublicGroupChatModel model;
		PublicGroupChatProxyModel proxy;
		auto modelData = [&proxy](Role role) {
			const int count = proxy.rowCount();
			QStringList result;

			result.reserve(count);

			for (int i = 0; i < count; ++i) {
				const QModelIndex index = proxy.index(i, 0);
				result.append(index.data(static_cast<int>(role)).toString());
			}

			return result;
		};

		proxy.setSourceModel(&model);

		model.setGroupChats({PublicGroupChat {QJsonObject {
					     {PublicGroupChat::Address.toString(), QStringLiteral("pasnox@jabber.com")},
					     {PublicGroupChat::Users.toString(), 42},
					     {PublicGroupChat::IsOpen.toString(), true},
					     {PublicGroupChat::Name.toString(), QStringLiteral("PasNox")},
					     {PublicGroupChat::Language.toString(), QStringLiteral("en")},
				     }},
			PublicGroupChat {QJsonObject {
				{PublicGroupChat::Address.toString(), QStringLiteral("bookri@jabber.com")},
				{PublicGroupChat::Users.toString(), 43},
				{PublicGroupChat::IsOpen.toString(), true},
				{PublicGroupChat::Name.toString(), QStringLiteral("Bookri")},
				{PublicGroupChat::Language.toString(), QStringLiteral("fr")},
			}},
			PublicGroupChat {QJsonObject {
				{PublicGroupChat::Address.toString(), QStringLiteral("zynox@jabber.com")},
				{PublicGroupChat::Users.toString(), 45},
				{PublicGroupChat::IsOpen.toString(), true},
				{PublicGroupChat::Name.toString(), QStringLiteral("ZyNox")},
				{PublicGroupChat::Language.toString(), QStringLiteral("de")},
			}}});

		QCOMPARE(proxy.sortColumn(), 0);

		QFETCH(Role, sortRole);
		QFETCH(Role, filterRole);
		QFETCH(QString, filterWildcard);
		QFETCH(QVector<QStringList>, expected);

		proxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
		proxy.setFilterRole(static_cast<int>(filterRole));
		proxy.setSortCaseSensitivity(Qt::CaseInsensitive);
		proxy.setSortRole(static_cast<int>(sortRole));
		proxy.setFilterWildcard(filterWildcard);

		QCOMPARE(model.count(), 3);
		QCOMPARE(proxy.count(), expected.constFirst().count());

		QCOMPARE(modelData(Role::Name), expected.at(0));
		QCOMPARE(modelData(Role::Address), expected.at(1));
		QCOMPARE(modelData(Role::Languages), expected.at(2));
	}
};

QTEST_GUILESS_MAIN(GroupChatTest)
#include "PublicGroupChatTest.moc"

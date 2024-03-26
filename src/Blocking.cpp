// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Blocking.h"
// Qt
#include <QSqlQuery>
// QXmpp
#include <QXmppBlockingManager.h>
#include <QXmppUtils.h>
// Kaidan
#include "AccountManager.h"
#include "Algorithms.h"
#include "FutureUtils.h"
#include "Kaidan.h"
#include "SqlUtils.h"

using namespace SqlUtils;

auto debug()
{
	return qDebug() << "[Blocking]";
}

auto determineJidType(const QString &jid)
{
	using Type = BlockingModel::JidType;

	auto user = QXmppUtils::jidToUser(jid);
	auto resource = QXmppUtils::jidToResource(jid);

	// domain is always set

	if (user.isEmpty()) {
		// no user set
		if (!resource.isEmpty()) {
			return Type::DomainResource;
		}
		return Type::Domain;
	}
	// user is set
	if (!resource.isEmpty()) {
		return Type::FullJid;
	}
	return Type::BareJid;
}

BlockingDb::BlockingDb(Database *database, QObject *parent) : DatabaseComponent(database, parent)
{
}

QFuture<QVector<QString>> BlockingDb::blockedJids(const QString &accountJid)
{
	return run([this, accountJid]() {
		auto query = createQuery();
		execQuery(query, QStringLiteral("SELECT jid FROM blocked WHERE accountJid = :accountJid"), { { u":accountJid", accountJid } });
		QVector<QString> blockedJids;
		while (query.next()) {
			blockedJids.append(query.value(0).toString());
		}
		return blockedJids;
	});
}

QFuture<void> BlockingDb::resetBlockedJids(const QString &accountJid, const QVector<QString> &blockedJids)
{
	return run([this, accountJid, blockedJids]() {
		auto query = createQuery();
		execQuery(query, QStringLiteral("DELETE FROM blocked WHERE accountJid = :accountJid"), { { u":accountJid", accountJid } });

		prepareQuery(query, QStringLiteral("INSERT INTO blocked (accountJid, jid) VALUES (:accountJid, :jid)"));
		for (const auto &jid : blockedJids) {
			bindValues(query, { { u":accountJid", accountJid }, { u":jid", jid } });
			execQuery(query);
		}
	});
}

QFuture<void> BlockingDb::removeBlockedJids(const QString &accountJid, const QVector<QString> &blockedJids)
{
	return run([this, accountJid, blockedJids]() {
		auto query = createQuery();
		prepareQuery(query, QStringLiteral("DELETE FROM blocked WHERE accountJid = :accountJid AND jid = :jid"));
		for (const auto &jid : blockedJids) {
			bindValues(query, { { u":accountJid", accountJid }, { u":jid", jid } });
			execQuery(query);
		}
	});
}

QFuture<void> BlockingDb::addBlockedJids(const QString &accountJid, const QVector<QString> &blockedJids)
{
	return run([this, accountJid, blockedJids]() {
		auto query = createQuery();
		prepareQuery(
			query, QStringLiteral("INSERT OR REPLACE INTO blocked (accountJid, jid) VALUES (:accountJid, :jid)"));
		for (const auto &jid : blockedJids) {
			bindValues(query, { { u":accountJid", accountJid }, { u":jid", jid } });
			execQuery(query);
		}
	});
}

BlockingController::BlockingController(Database *database, QObject *parent)
	: QObject(parent), m_db(std::make_unique<BlockingDb>(database))
{
}

void BlockingController::registerModel(BlockingModel *model)
{
	m_registeredModels.push_back(model);
	if (m_subscribed) {
		// reset data of the model if data is available
		if (m_blocklist) {
			model->handleReset(m_blocklist->jids);
		}
	} else {
		// request data from the server (or db)
		subscribeToBlocklist();
	}
}

void BlockingController::unregisterModel(BlockingModel *model)
{
	m_registeredModels.erase(std::find(m_registeredModels.begin(), m_registeredModels.end(), model));
}

void BlockingController::subscribeToBlocklist()
{
	if (m_subscribed) {
		return;
	}
	m_subscribed = true;

	// load blocklist from database
	await(m_db->blockedJids(AccountManager::instance()->jid()), this, [this](auto result) {
		handleBlocklist({ Blocklist::Db, std::move(result) });
	});

	auto *client = Kaidan::instance()->client();

	// also load blocklist from XMPP if connected
	if (Kaidan::instance()->connected()) {
		callRemoteTask(
			client,
			[]() {
				auto *manager = Kaidan::instance()->client()->xmppClient()->findExtension<QXmppBlockingManager>();
				Q_ASSERT(manager);

				return std::pair { manager->fetchBlocklist(), manager };
			},
			this,
			[this](auto result) { handleXmppBlocklistResult(std::move(result)); });
	}

	// connect to signals
	runOnThread(client, [this]() {
		auto *manager = Kaidan::instance()->client()->xmppClient()->findExtension<QXmppBlockingManager>();

		connect(manager, &QXmppBlockingManager::blocked, this, &BlockingController::onJidsBlocked);
		connect(manager, &QXmppBlockingManager::unblocked, this, &BlockingController::onJidsUnblocked);
	});

	// re-subscribe to blocklist on reconnect (without stream resumption)
	connect(client, &ClientWorker::connectionStateChanged, this, [this](auto connState) {
		if (connState != Enums::ConnectionState::StateConnected) {
			return;
		}

		// connected again
		runOnThread(Kaidan::instance()->client(), [this]() {
			auto *manager = Kaidan::instance()->client()->xmppClient()->findExtension<QXmppBlockingManager>();
			if (manager->isSubscribed()) {
				return;
			}

			manager->fetchBlocklist().then(this, [this](auto result) {
				runOnThread(this, [this, result = std::move(result)]() mutable {
					handleXmppBlocklistResult(std::move(result));
				});
			});
		});
	});
}

std::optional<QXmppBlocklist> BlockingController::blocklist() const
{
	if (m_blocklist) {
		return QXmppBlocklist(m_blocklist->jids);
	}
	return {};
}

void BlockingController::handleXmppBlocklistResult(QXmppBlockingManager::BlocklistResult &&result)
{
	if (auto error = std::get_if<QXmppError>(&result)) {
		// avoid resetting cache when a connection error occurred
		if (error->isStanzaError()) {
			handleBlocklist({ Blocklist::Xmpp, QVector<QString>() });
		}

		Q_EMIT Kaidan::instance()->passiveNotificationRequested(
			tr("Error fetching blocklist: %1").arg(error->description));
		debug() << "Error fetching blocklist:" << error->description;
	} else {
		// success
		handleBlocklist({ Blocklist::Xmpp, std::get<QXmppBlocklist>(result).entries() });
	}
}

void BlockingController::handleBlocklist(Blocklist blocklist)
{
	// Do not update the blocklist from the server to the version in the database.
	if (!m_blocklist || m_blocklist->source <= blocklist.source) {
		// update database
		if (blocklist.source == Blocklist::Xmpp) {
			updateDbBlocklist(blocklist.jids);
		}

		// whether the new blocklist differs
		bool changed = !m_blocklist || (m_blocklist && m_blocklist->jids != blocklist.jids);

		m_blocklist = std::move(blocklist);

		// notify handlers
		if (changed) {
			for (auto *model : m_registeredModels) {
				model->handleReset(m_blocklist->jids);
			}

			Q_EMIT blocklistChanged();
		}
	}
}

void BlockingController::updateDbBlocklist(const QVector<QString> &blockedJids)
{
	if (m_blocklist && m_blocklist->source == Blocklist::Db && m_blocklist->jids == blockedJids) {
		// skip if we already know that the db version is up to date
		return;
	}
	m_db->resetBlockedJids(AccountManager::instance()->jid(), blockedJids);
}

void BlockingController::onJidsBlocked(const QVector<QString> &jids)
{
	Q_ASSERT(m_blocklist);
	Q_ASSERT(m_blocklist->source == Blocklist::Xmpp);

	m_db->addBlockedJids(AccountManager::instance()->jid(), jids);

	m_blocklist->jids.append(jids);
	makeUnique(m_blocklist->jids);

	// handler
	for (auto *model : m_registeredModels) {
		model->handleBlocked(jids);
	}
	Q_EMIT blocklistChanged();
}

void BlockingController::onJidsUnblocked(const QVector<QString> &jids)
{
	Q_ASSERT(m_blocklist);
	Q_ASSERT(m_blocklist->source == Blocklist::Xmpp);

	m_db->removeBlockedJids(AccountManager::instance()->jid(), jids);

	for (const auto &jid : jids) {
		m_blocklist->jids.removeOne(jid);
	}

	// handler
	for (auto *model : m_registeredModels) {
		model->handleUnblocked(jids);
	}
	Q_EMIT blocklistChanged();
}

BlockingModel::BlockingModel(QObject *parent) : QAbstractListModel(parent)
{
	Kaidan::instance()->blockingController()->registerModel(this);
}

BlockingModel::~BlockingModel()
{
	Kaidan::instance()->blockingController()->unregisterModel(this);
}

QHash<int, QByteArray> BlockingModel::roleNames() const
{
	static const QHash<int, QByteArray> roles = {
		{static_cast<int>(Role::Jid), QByteArrayLiteral("jid")},
		{static_cast<int>(Role::Type), QByteArrayLiteral("type")},
	};

	return roles;
}

int BlockingModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	}
	return m_entries.size();
}

QVariant BlockingModel::data(const QModelIndex &index, int role) const
{
	if (!hasIndex(index.row(), index.column(), index.parent())) {
		return {};
	}

	const auto &entry = m_entries.at(index.row());
	switch (static_cast<Role>(role)) {
	case Role::Jid:
		return entry.jid;
	case Role::Type:
		switch (entry.type) {
		case JidType::FullJid:
			return tr("Devices");
		case JidType::BareJid:
			return tr("Users");
		case JidType::Domain:
			return tr("Servers");
		case JidType::DomainResource:
			return tr("Devices on Servers");
		}
		break;
	default:
		break;
	}
	return {};
}

bool BlockingModel::contains(const QString &jid)
{
	return std::find_if(m_entries.cbegin(), m_entries.cend(), [&](const auto &e) {
		return e.jid == jid;
	}) != m_entries.cend();
}

void BlockingModel::handleReset(const QVector<QString> &jids)
{
	beginResetModel();
	m_entries = transform(jids, [](const QString &jid) {
		return Entry { determineJidType(jid), jid };
	});
	std::sort(m_entries.begin(), m_entries.end());
	endResetModel();
}

void BlockingModel::handleBlocked(const QVector<QString> &jids)
{
	for (const auto &jid : jids) {
		Entry e { determineJidType(jid), jid };

		auto *itr = std::upper_bound(m_entries.begin(), m_entries.end(), e);
		auto index = std::distance(m_entries.begin(), itr);

		beginInsertRows({}, index, index);
		m_entries.insert(itr, std::move(e));
		endInsertRows();
	}
}

void BlockingModel::handleUnblocked(const QVector<QString> &jids)
{
	for (const auto &jid : jids) {
		auto *itr = std::find_if(m_entries.begin(), m_entries.end(), [&](const auto &e) {
			return e.jid == jid;
		});
		auto index = std::distance(m_entries.begin(), itr);

		beginRemoveRows({}, index, index);
		m_entries.erase(itr);
		endRemoveRows();
	}
}

BlockingWatcher::BlockingWatcher(QObject *parent) : QObject(parent)
{
	auto *controller = Kaidan::instance()->blockingController();
	connect(controller, &BlockingController::blocklistChanged, this, &BlockingWatcher::refreshState);
}

const QString &BlockingWatcher::jid() const
{
	return m_jid;
}

void BlockingWatcher::setJid(const QString &jid)
{
	if (!jid.isEmpty()) {
		Kaidan::instance()->blockingController()->subscribeToBlocklist();
	}

	if (m_jid != jid) {
		m_jid = jid;
		Q_EMIT jidChanged();

		refreshState();
	}
}

bool BlockingWatcher::blocked() const
{
	// partially blocked (e.g., some devices blocked) is not interpreted as "blocked"
	return std::holds_alternative<QXmppBlocklist::Blocked>(m_state);
}

void BlockingWatcher::refreshState()
{
	auto blockedOld = blocked();

	// get current blocklist
	if (auto list = Kaidan::instance()->blockingController()->blocklist()) {
		m_state = list->blockingState(m_jid);
	} else {
		m_state = QXmppBlocklist::NotBlocked();
	}
	Q_EMIT stateChanged();

	if (blockedOld != blocked()) {
		Q_EMIT blockedChanged();
	}
}

BlockingAction::BlockingAction(QObject *parent) : QObject(parent)
{
}

void BlockingAction::block(const QString &jid)
{
	debug() << "Blocking" << jid;
	setRunning(m_running + 1);
	callRemoteTask(
		Kaidan::instance()->client(),
		[this, jid]() {
			auto *manager = Kaidan::instance()->client()->xmppClient()->findExtension<QXmppBlockingManager>();
			return std::pair { manager->block(jid), this };
		},
		this,
		[this, jid](auto result) {
			if (auto *error = std::get_if<QXmppError>(&result)) {
				Q_EMIT errorOccurred(jid, true, error->description);
			} else {
				debug() << "Blocked" << jid;
				Q_EMIT succeeded(jid, true);
			}

			setRunning(m_running - 1);
		});
}

void BlockingAction::unblock(const QString &jid)
{
	debug() << "Unblocking" << jid;
	setRunning(m_running + 1);
	callRemoteTask(
		Kaidan::instance()->client(),
		[this, jid]() {
			auto *manager = Kaidan::instance()->client()->xmppClient()->findExtension<QXmppBlockingManager>();
			return std::pair { manager->unblock(jid), this };
		},
		this,
		[this, jid](auto result) {
			if (auto *error = std::get_if<QXmppError>(&result)) {
				Q_EMIT errorOccurred(jid, false, error->description);
			} else {
				debug() << "Unblocked" << jid;
				Q_EMIT succeeded(jid, false);
			}

			setRunning(m_running - 1);
		});
}

void BlockingAction::setRunning(uint running)
{
	auto loadingOld = loading();
	m_running = running;

	if (loading() != loadingOld) {
		Q_EMIT loadingChanged();
	}
}

// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Blocking.h"

// Qt
#include <QSqlQuery>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "Algorithms.h"
#include "ClientWorker.h"
#include "Globals.h"
#include "KaidanCoreLog.h"
#include "SqlUtils.h"

using namespace SqlUtils;

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

BlockingDb::BlockingDb(QObject *parent)
    : DatabaseComponent(parent)
{
}

QFuture<QList<QString>> BlockingDb::blockedJids(const QString &accountJid)
{
    return run([this, accountJid]() {
        auto query = createQuery();
        execQuery(query, QStringLiteral("SELECT jid FROM blocked WHERE accountJid = :accountJid"), {{u":accountJid", accountJid}});
        QList<QString> blockedJids;
        while (query.next()) {
            blockedJids.append(query.value(0).toString());
        }
        return blockedJids;
    });
}

QFuture<void> BlockingDb::resetBlockedJids(const QString &accountJid, const QList<QString> &blockedJids)
{
    return run([this, accountJid, blockedJids]() {
        auto query = createQuery();
        execQuery(query, QStringLiteral("DELETE FROM blocked WHERE accountJid = :accountJid"), {{u":accountJid", accountJid}});

        for (const auto &jid : blockedJids) {
            insert(QString::fromLatin1(DB_TABLE_BLOCKED),
                   {
                       {u"accountJid", accountJid},
                       {u"jid", jid},
                   });
        }
    });
}

QFuture<void> BlockingDb::removeBlockedJids(const QString &accountJid, const QList<QString> &blockedJids)
{
    return run([this, accountJid, blockedJids]() {
        auto query = createQuery();
        prepareQuery(query, QStringLiteral("DELETE FROM blocked WHERE accountJid = :accountJid AND jid = :jid"));
        for (const auto &jid : blockedJids) {
            bindValues(query, {{u":accountJid", accountJid}, {u":jid", jid}});
            execQuery(query);
        }
    });
}

QFuture<void> BlockingDb::addBlockedJids(const QString &accountJid, const QList<QString> &blockedJids)
{
    return run([this, accountJid, blockedJids]() {
        auto query = createQuery();
        prepareQuery(query, QStringLiteral("INSERT OR REPLACE INTO blocked (accountJid, jid) VALUES (:accountJid, :jid)"));
        for (const auto &jid : blockedJids) {
            bindValues(query, {{u":accountJid", accountJid}, {u":jid", jid}});
            execQuery(query);
        }
    });
}

BlockingController::BlockingController(AccountSettings *accountSettings, Connection *connection, ClientWorker *clientWorker, QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_connection(connection)
    , m_clientWorker(clientWorker)
    , m_manager(m_clientWorker->blockingManager())
    , m_db(std::make_unique<BlockingDb>())
{
}

void BlockingController::subscribeToBlocklist()
{
    if (m_subscribed) {
        return;
    }
    m_subscribed = true;

    // load blocklist from database
    m_db->blockedJids(m_accountSettings->jid()).then(this, [this](QList<QString> result) {
        handleBlocklist({Blocklist::Db, std::move(result)});
    });

    // also load blocklist from XMPP if connected
    if (m_connection->state() == Enums::ConnectionState::StateConnected) {
        callRemoteTask(
            m_manager,
            [this]() {
                return std::pair{m_manager->fetchBlocklist(), this};
            },
            this,
            [this](auto &&result) {
                handleXmppBlocklistResult(std::move(result));
            });
    }

    // connect to signals
    runOnThread(m_manager, [this]() {
        connect(m_manager, &QXmppBlockingManager::blocked, this, &BlockingController::onJidsBlocked);
        connect(m_manager, &QXmppBlockingManager::unblocked, this, &BlockingController::onJidsUnblocked);
    });

    // re-subscribe to blocklist on reconnect (without stream resumption)
    connect(m_clientWorker, &ClientWorker::connectionStateChanged, this, [this](auto connState) {
        if (connState != Enums::ConnectionState::StateConnected) {
            return;
        }

        // connected again
        runOnThread(m_clientWorker, [this]() {
            if (m_manager->isSubscribed()) {
                return;
            }

            m_manager->fetchBlocklist().then(this, [this](auto result) {
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

void BlockingController::block(const QString &jid)
{
    qCDebug(KAIDAN_CORE_LOG) << "Blocking" << jid;
    setRunning(m_runningActionCount + 1);
    callRemoteTask(
        m_manager,
        [this, jid]() {
            return std::pair{m_manager->block(jid), this};
        },
        this,
        [this, jid](auto result) {
            if (auto *error = std::get_if<QXmppError>(&result)) {
                Q_EMIT blockingFailed(jid, error->description);
            } else {
                qCDebug(KAIDAN_CORE_LOG) << "Blocked" << jid;
                Q_EMIT blocked(jid);
            }

            setRunning(m_runningActionCount - 1);
        });
}

void BlockingController::unblock(const QString &jid)
{
    qCDebug(KAIDAN_CORE_LOG) << "Unblocking" << jid;
    setRunning(m_runningActionCount + 1);
    callRemoteTask(
        m_manager,
        [this, jid]() {
            return std::pair{m_manager->unblock(jid), this};
        },
        this,
        [this, jid](auto result) {
            if (auto *error = std::get_if<QXmppError>(&result)) {
                Q_EMIT unblockingFailed(jid, error->description);
            } else {
                qCDebug(KAIDAN_CORE_LOG) << "Unblocked" << jid;
                Q_EMIT unblocked(jid);
            }

            setRunning(m_runningActionCount - 1);
        });
}

bool BlockingController::busy() const
{
    return m_runningActionCount;
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

void BlockingController::setRunning(uint running)
{
    auto loadingOld = busy();
    m_runningActionCount = running;

    if (busy() != loadingOld) {
        Q_EMIT busyChanged();
    }
}

void BlockingController::handleXmppBlocklistResult(QXmppBlockingManager::BlocklistResult &&result)
{
    if (auto error = std::get_if<QXmppError>(&result)) {
        // avoid resetting cache when a connection error occurred
        if (error->isStanzaError()) {
            handleBlocklist({Blocklist::Xmpp, QList<QString>()});
        }

        qCDebug(KAIDAN_CORE_LOG) << "Error fetching blocklist:" << error->description;
    } else {
        // success
        handleBlocklist({Blocklist::Xmpp, std::get<QXmppBlocklist>(result).entries()});
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

void BlockingController::updateDbBlocklist(const QList<QString> &blockedJids)
{
    if (m_blocklist && m_blocklist->source == Blocklist::Db && m_blocklist->jids == blockedJids) {
        // skip if we already know that the db version is up to date
        return;
    }
    m_db->resetBlockedJids(m_accountSettings->jid(), blockedJids);
}

void BlockingController::onJidsBlocked(const QList<QString> &jids)
{
    Q_ASSERT(m_blocklist);
    Q_ASSERT(m_blocklist->source == Blocklist::Xmpp);

    m_db->addBlockedJids(m_accountSettings->jid(), jids);

    m_blocklist->jids.append(jids);
    makeUnique(m_blocklist->jids);

    // handler
    for (auto *model : m_registeredModels) {
        model->handleBlocked(jids);
    }
    Q_EMIT blocklistChanged();
}

void BlockingController::onJidsUnblocked(const QList<QString> &jids)
{
    Q_ASSERT(m_blocklist);
    Q_ASSERT(m_blocklist->source == Blocklist::Xmpp);

    m_db->removeBlockedJids(m_accountSettings->jid(), jids);

    for (const auto &jid : jids) {
        m_blocklist->jids.removeOne(jid);
    }

    // handler
    for (auto *model : m_registeredModels) {
        model->handleUnblocked(jids);
    }
    Q_EMIT blocklistChanged();
}

BlockingModel::BlockingModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

BlockingModel::~BlockingModel()
{
    m_blockingController->unregisterModel(this);
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

void BlockingModel::setBlockingController(BlockingController *blockingController)
{
    if (m_blockingController != blockingController) {
        if (m_blockingController) {
            m_blockingController->unregisterModel(this);
        }

        m_blockingController = blockingController;
        m_blockingController->registerModel(this);
    }
}

bool BlockingModel::contains(const QString &jid)
{
    return std::find_if(m_entries.cbegin(),
                        m_entries.cend(),
                        [&](const auto &e) {
                            return e.jid == jid;
                        })
        != m_entries.cend();
}

void BlockingModel::handleReset(const QList<QString> &jids)
{
    beginResetModel();
    m_entries = transform(jids, [](const QString &jid) {
        return Entry{determineJidType(jid), jid};
    });
    std::sort(m_entries.begin(), m_entries.end());
    endResetModel();
}

void BlockingModel::handleBlocked(const QList<QString> &jids)
{
    for (const auto &jid : jids) {
        Entry e{determineJidType(jid), jid};

        auto itr = std::upper_bound(m_entries.begin(), m_entries.end(), e);
        auto index = std::distance(m_entries.begin(), itr);

        beginInsertRows({}, index, index);
        m_entries.insert(itr, std::move(e));
        endInsertRows();
    }
}

void BlockingModel::handleUnblocked(const QList<QString> &jids)
{
    for (const auto &jid : jids) {
        auto itr = std::find_if(m_entries.begin(), m_entries.end(), [&](const auto &e) {
            return e.jid == jid;
        });
        auto index = std::distance(m_entries.begin(), itr);

        beginRemoveRows({}, index, index);
        m_entries.erase(itr);
        endRemoveRows();
    }
}

BlockingWatcher::BlockingWatcher(QObject *parent)
    : QObject(parent)
{
}

void BlockingWatcher::setBlockingController(BlockingController *blockingController)
{
    if (m_blockingController != blockingController) {
        if (m_blockingController) {
            disconnect(m_blockingController, &BlockingController::blocklistChanged, this, &BlockingWatcher::refreshState);
        }

        m_blockingController = blockingController;
        connect(m_blockingController, &BlockingController::blocklistChanged, this, &BlockingWatcher::refreshState);
    }
}

const QString &BlockingWatcher::jid() const
{
    return m_jid;
}

void BlockingWatcher::setJid(const QString &jid)
{
    if (!jid.isEmpty()) {
        m_blockingController->subscribeToBlocklist();
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
    if (auto list = m_blockingController->blocklist(); list && !m_jid.isEmpty()) {
        m_state = list->blockingState(m_jid);
    } else {
        m_state = QXmppBlocklist::NotBlocked();
    }
    Q_EMIT stateChanged();

    if (blockedOld != blocked()) {
        Q_EMIT blockedChanged();
    }
}

#include "moc_Blocking.cpp"

// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>
// QXmpp
#include <QXmppBlockingManager.h>
// Kaidan
#include "DatabaseComponent.h"

class AccountSettings;
class Connection;
class BlockingModel;
class BlockingWatcher;
class ClientWorker;

class BlockingDb : public DatabaseComponent
{
    Q_OBJECT
public:
    explicit BlockingDb(QObject *parent = nullptr);

    QFuture<QList<QString>> blockedJids(const QString &accountJid);
    QFuture<void> resetBlockedJids(const QString &accountJid, const QList<QString> &blockedJids);
    QFuture<void> removeBlockedJids(const QString &accountJid, const QList<QString> &blockedJids);
    QFuture<void> addBlockedJids(const QString &accountJid, const QList<QString> &blockedJids);
};

// Coordinates loading and updates of the blocklist
class BlockingController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    BlockingController(AccountSettings *accountSettings, Connection *connection, ClientWorker *clientWorker, QObject *parent = nullptr);

    void subscribeToBlocklist();
    [[nodiscard]] std::optional<QXmppBlocklist> blocklist() const;
    Q_SIGNAL void blocklistChanged();

    Q_INVOKABLE void block(const QString &jid);
    Q_SIGNAL void blocked(const QString &jid);
    Q_SIGNAL void blockingFailed(const QString &jid, const QString &errorText);

    Q_INVOKABLE void unblock(const QString &jid);
    Q_SIGNAL void unblocked(const QString &jid);
    Q_SIGNAL void unblockingFailed(const QString &jid, const QString &errorText);

    [[nodiscard]] bool busy() const;
    Q_SIGNAL void busyChanged();

    void registerModel(BlockingModel *model);
    void unregisterModel(BlockingModel *model);

private:
    struct Blocklist {
        enum {
            Db,
            Xmpp,
        } source;

        QList<QString> jids;
    };

    void setRunning(uint);

    void handleXmppBlocklistResult(QXmppBlockingManager::BlocklistResult &&result);
    void handleBlocklist(Blocklist blocklist);
    void updateDbBlocklist(const QList<QString> &blockedJids);

    void onJidsBlocked(const QList<QString> &blockedJids);
    void onJidsUnblocked(const QList<QString> &unblockedJids);

    AccountSettings *const m_accountSettings;
    Connection *const m_connection;
    ClientWorker *const m_clientWorker;
    QXmppBlockingManager *const m_manager;

    bool m_subscribed = false;
    std::unique_ptr<BlockingDb> m_db;
    std::optional<Blocklist> m_blocklist;
    uint m_runningActionCount = 0;
    std::vector<BlockingModel *> m_registeredModels;
};

// Provides access to all blocked JIDs
class BlockingModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(BlockingController *blockingController MEMBER m_blockingController WRITE setBlockingController)

    friend class BlockingController;

public:
    enum class JidType {
        Domain,
        BareJid,
        FullJid,
        DomainResource,
    };
    Q_ENUM(JidType)

    enum class Role {
        Jid = Qt::DisplayRole,
        Type = Qt::UserRole + 1,
    };
    Q_ENUM(Role)

    explicit BlockingModel(QObject *parent = nullptr);
    ~BlockingModel() override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setBlockingController(BlockingController *blockingController);

    Q_INVOKABLE bool contains(const QString &jid);

private:
    void handleReset(const QList<QString> &);
    void handleBlocked(const QList<QString> &);
    void handleUnblocked(const QList<QString> &);

    struct Entry {
        JidType type;
        QString jid;

        auto operator<=>(const Entry &other) const
        {
            if (type != other.type) {
                return static_cast<int>(type) - static_cast<int>(other.type);
            }
            return jid.compare(other.jid);
        }
    };

    BlockingController *m_blockingController = nullptr;
    QList<Entry> m_entries;
};

// Watches the blocking state of a single JID
class BlockingWatcher : public QObject
{
    Q_OBJECT

    Q_PROPERTY(BlockingController *blockingController MEMBER m_blockingController WRITE setBlockingController)
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(bool blocked READ blocked NOTIFY blockedChanged)

public:
    explicit BlockingWatcher(QObject *parent = nullptr);

    void setBlockingController(BlockingController *blockingController);

    [[nodiscard]] const QString &jid() const;
    void setJid(const QString &jid);
    Q_SIGNAL void jidChanged();

    [[nodiscard]] bool blocked() const;
    Q_SIGNAL void blockedChanged();

    Q_SIGNAL void stateChanged();

private:
    void refreshState();

    BlockingController *m_blockingController = nullptr;
    QString m_jid;
    QXmppBlocklist::BlockingState m_state = QXmppBlocklist::NotBlocked();
};

Q_DECLARE_METATYPE(BlockingModel::JidType)
Q_DECLARE_METATYPE(BlockingModel::Role)

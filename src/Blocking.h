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

class ClientWorker;
class BlockingModel;
class BlockingWatcher;

class BlockingDb : public DatabaseComponent
{
	Q_OBJECT
public:
	BlockingDb(Database *database, QObject *parent = nullptr);

	QFuture<QVector<QString>> blockedJids(const QString &accountJid);
	QFuture<void> resetBlockedJids(const QString &accountJid, const QVector<QString> &blockedJids);
	QFuture<void> removeBlockedJids(const QString &accountJid, const QVector<QString> &blockedJids);
	QFuture<void> addBlockedJids(const QString &accountJid, const QVector<QString> &blockedJids);
};

// Coordinates loading and updates of the blocklist
class BlockingController : public QObject
{
	Q_OBJECT
public:
	explicit BlockingController(Database *database, QObject *parent = nullptr);

	void registerModel(BlockingModel *model);
	void unregisterModel(BlockingModel *model);

	void subscribeToBlocklist();
	[[nodiscard]] std::optional<QXmppBlocklist> blocklist() const;
	Q_SIGNAL void blocklistChanged();

private:
	struct Blocklist {
		enum { Db, Xmpp } source;
		QVector<QString> jids;
	};

	void handleXmppBlocklistResult(QXmppBlockingManager::BlocklistResult &&result);
	void handleBlocklist(Blocklist blocklist);
	void updateDbBlocklist(const QVector<QString> &blockedJids);
	void onJidsBlocked(const QVector<QString> &blockedJids);
	void onJidsUnblocked(const QVector<QString> &unblockedJids);

	bool m_subscribed = false;
	std::unique_ptr<BlockingDb> m_db;
	std::optional<Blocklist> m_blocklist;
	std::vector<BlockingModel *> m_registeredModels;
};

// Provides access to all blocked JIDs
class BlockingModel : public QAbstractListModel
{
	Q_OBJECT
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

	Q_INVOKABLE bool contains(const QString &jid);

private:
	void handleReset(const QVector<QString> &);
	void handleBlocked(const QVector<QString> &);
	void handleUnblocked(const QVector<QString> &);

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

	QVector<Entry> m_entries;
};

// Watches the blocking state of a single JID
class BlockingWatcher : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
	Q_PROPERTY(bool blocked READ blocked NOTIFY blockedChanged)

public:
	explicit BlockingWatcher(QObject *parent = nullptr);

	[[nodiscard]] const QString &jid() const;
	void setJid(const QString &jid);
	Q_SIGNAL void jidChanged();

	[[nodiscard]] bool blocked() const;
	Q_SIGNAL void blockedChanged();

	Q_SIGNAL void stateChanged();

private:
	void refreshState();

	QString m_jid;
	QXmppBlocklist::BlockingState m_state = QXmppBlocklist::NotBlocked();
};

// Used for (un)blocking JIDs from QML
class BlockingAction : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
	explicit BlockingAction(QObject *parent = nullptr);

	Q_INVOKABLE void block(const QString &jid);
	Q_INVOKABLE void unblock(const QString &jid);

	Q_SIGNAL void succeeded(const QString &jid, bool block);
	Q_SIGNAL void errorOccurred(const QString &jid, bool block, const QString &errorText);

	[[nodiscard]] bool loading() const
	{
		return m_running > 0;
	}
	Q_SIGNAL void loadingChanged();

private:
	void setRunning(uint);
	uint m_running = 0;
};

Q_DECLARE_METATYPE(BlockingModel::JidType)
Q_DECLARE_METATYPE(BlockingModel::Role)

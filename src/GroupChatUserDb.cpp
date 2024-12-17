// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatUserDb.h"

// Qt
#include <QDateTime>
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QString>
// Kaidan
#include "Database.h"
#include "Globals.h"
#include "GroupChatUser.h"
#include "MessageDb.h"
#include "SqlUtils.h"

using namespace SqlUtils;

constexpr QStringView ACCOUNT_JID = u"accountJid";
constexpr QStringView CHAT_JID = u"chatJid";
constexpr QStringView ID = u"id";
constexpr QStringView JID = u"jid";
constexpr QStringView NAME = u"name";
constexpr QStringView STATUS = u"status";

GroupChatUserDb *GroupChatUserDb::s_instance = nullptr;

GroupChatUserDb::GroupChatUserDb(Database *db, QObject *parent)
    : DatabaseComponent(db, parent)
{
    Q_ASSERT(!GroupChatUserDb::s_instance);
    s_instance = this;
}

GroupChatUserDb::~GroupChatUserDb()
{
    s_instance = nullptr;
}

GroupChatUserDb *GroupChatUserDb::instance()
{
    return s_instance;
}

QFuture<std::optional<GroupChatUser>> GroupChatUserDb::user(const QString &accountJid, const QString &chatJid, const QString &participantId)
{
    return run([this, accountJid, chatJid, participantId]() {
        return _user(accountJid, chatJid, participantId);
    });
}

std::optional<GroupChatUser> GroupChatUserDb::_user(const QString &accountJid, const QString &chatJid, const QString &participantId)
{
    auto query = createQuery();
    query.setForwardOnly(true);

    QMap<QString, QVariant> keyValuePairs = {{ACCOUNT_JID.toString(), accountJid}, {CHAT_JID.toString(), chatJid}, {ID.toString(), participantId}};

    execQuery(query,
              QStringLiteral("SELECT * FROM " DB_TABLE_GROUP_CHAT_USERS) + simpleWhereStatement(&sqlDriver(), keyValuePairs)
                  + QStringLiteral(" "
                                   "LIMIT 1"));

    QList<GroupChatUser> users;
    parseUsersFromQuery(query, users);

    if (users.isEmpty()) {
        return std::nullopt;
    }

    return users.constFirst();
}

QFuture<QList<GroupChatUser>> GroupChatUserDb::users(const QString &accountJid, const QString &chatJid, const int offset)
{
    return run([this, accountJid, chatJid, offset]() {
        auto query = createQuery();
        query.setForwardOnly(true);

        QMap<QString, QVariant> keyValuePairs = {{ACCOUNT_JID.toString(), accountJid}, {CHAT_JID.toString(), chatJid}};

        execQuery(query,
                  QStringLiteral("SELECT * FROM " DB_TABLE_GROUP_CHAT_USERS) + simpleWhereStatement(&sqlDriver(), keyValuePairs)
                      + QStringLiteral(" "
                                       "ORDER BY ")
                      + NAME.toString() + ", " + STATUS.toString()
                      + QStringLiteral(" "
                                       "LIMIT %1, %2")
                            .arg(offset)
                            .arg(DB_QUERY_LIMIT_GROUP_CHAT_USERS));

        QList<GroupChatUser> users;
        parseUsersFromQuery(query, users);

        return users;
    });
}

QFuture<QList<QString>> GroupChatUserDb::userJids(const QString &accountJid, const QString &chatJid)
{
    return run([this, accountJid, chatJid]() {
        auto query = createQuery();
        query.setForwardOnly(true);

        QMap<QString, QVariant> keyValuePairs = {{ACCOUNT_JID.toString(), accountJid}, {CHAT_JID.toString(), chatJid}};

        execQuery(query,
                  QStringLiteral("SELECT ") + JID.toString() + QStringLiteral(" FROM " DB_TABLE_GROUP_CHAT_USERS)
                      + simpleWhereStatement(&sqlDriver(), keyValuePairs));

        QList<QString> userJids;

        while (query.next()) {
            if (const auto jid = query.value(0).toString(); !jid.isEmpty()) {
                userJids.append(jid);
            }
        }

        return userJids;
    });
}

QFuture<void> GroupChatUserDb::handleUserAllowedOrBanned(const GroupChatUser &user)
{
    return run([this, user]() {
        auto query = createQuery();

        QMap<QString, QVariant> keyValuePairs = {{ACCOUNT_JID.toString(), user.accountJid}, {CHAT_JID.toString(), user.chatJid}, {JID.toString(), user.jid}};

        execQuery(query,
                  QStringLiteral("SELECT 1 FROM " DB_TABLE_GROUP_CHAT_USERS) + simpleWhereStatement(&sqlDriver(), keyValuePairs)
                      + QStringLiteral(" "
                                       "LIMIT 1"));

        // If there is a stored user with a different status, update that user.
        // Otherwise, add a new entry.
        // "INSERT OR IGNORE INTO" is not possible because the columns that are checked differ from
        // those that are inserted.
        if (query.next()) {
            updateUserByJid(user.accountJid, user.chatJid, user.jid, [status = user.status](GroupChatUser &storedUser) {
                storedUser.status = status;
            });
        } else {
            addUser(user);
        }
    });
}

QFuture<void> GroupChatUserDb::handleUserDisallowedOrUnbanned(const GroupChatUser &user)
{
    return run([this, user]() {
        auto query = createQuery();

        QMap<QString, QVariant> keyValuePairs = {{ACCOUNT_JID.toString(), user.accountJid},
                                                 {CHAT_JID.toString(), user.chatJid},
                                                 {JID.toString(), user.jid},
                                                 {STATUS.toString(), int(user.status)}};

        execQuery(query,
                  QStringLiteral("SELECT 1 FROM ") + simpleWhereStatement(&sqlDriver(), keyValuePairs)
                      + QStringLiteral(" "
                                       "LIMIT 1"));

        if (query.next()) {
            removeUser(user);
        }
    });
}

QFuture<void> GroupChatUserDb::handleParticipantReceived(GroupChatUser participant)
{
    return run([this, participant]() mutable {
        auto query = createQuery();

        QMap<QString, QVariant> keyValuePairs = {{ACCOUNT_JID.toString(), participant.accountJid},
                                                 {CHAT_JID.toString(), participant.chatJid},
                                                 {ID.toString(), participant.id}};

        execQuery(query,
                  QStringLiteral("SELECT 1 FROM " DB_TABLE_GROUP_CHAT_USERS) + simpleWhereStatement(&sqlDriver(), keyValuePairs)
                      + QStringLiteral(" "
                                       "LIMIT 1"));

        participant.status = GroupChatUser::Status::Joined;

        // If the participant was already joined but modified, update the former entry normally.
        // If the participant was set as allowed to join but not yet joined, transform the former
        // entry to a joined one.
        // If the participant was not set as allowed before and joined now, add the participant.
        if (query.next()) {
            updateUserById(participant.accountJid, participant.chatJid, participant.id, [participant](GroupChatUser &user) {
                user.name = participant.name;
                user.status = participant.status;
            });
        } else {
            keyValuePairs = {{ACCOUNT_JID.toString(), participant.accountJid}, {CHAT_JID.toString(), participant.chatJid}, {JID.toString(), participant.jid}};

            execQuery(query,
                      QStringLiteral("SELECT 1 FROM " DB_TABLE_GROUP_CHAT_USERS) + simpleWhereStatement(&sqlDriver(), keyValuePairs)
                          + QStringLiteral(" "
                                           "LIMIT 1"));

            if (query.next()) {
                updateUserByJid(participant.accountJid, participant.chatJid, participant.jid, [participant](GroupChatUser &user) {
                    user.id = participant.id;
                    user.name = participant.name;
                    user.status = participant.status;
                });
            } else {
                addUser(participant);
            }
        }
    });
}

QFuture<void> GroupChatUserDb::handleParticipantLeft(const GroupChatUser &participant)
{
    return run([this, participant]() {
        auto query = createQuery();

        QMap<QString, QVariant> keyValuePairs = {{ACCOUNT_JID.toString(), participant.accountJid},
                                                 {CHAT_JID.toString(), participant.chatJid},
                                                 {ID.toString(), participant.id}};

        execQuery(query,
                  QStringLiteral("SELECT ") + STATUS.toString() + QStringLiteral(" FROM " DB_TABLE_GROUP_CHAT_USERS)
                      + simpleWhereStatement(&sqlDriver(), keyValuePairs)
                      + QStringLiteral(" "
                                       "LIMIT 1"));

        if (query.next()) {
            if (const auto status = query.value(0).value<GroupChatUser::Status>();
                status == GroupChatUser::Status::Allowed || MessageDb::instance()->_hasMessage(participant.accountJid, participant.chatJid, participant.id)) {
                updateUserById(participant.accountJid, participant.chatJid, participant.id, [](GroupChatUser &user) {
                    user.status = GroupChatUser::Status::Left;
                });
            } else {
                removeUser(participant);
            }
        }
    });
}

QFuture<void> GroupChatUserDb::handleMessageSender(GroupChatUser sender)
{
    return run([this, sender]() mutable {
        auto query = createQuery();

        QMap<QString, QVariant> keyValuePairs = {{ACCOUNT_JID.toString(), sender.accountJid},
                                                 {CHAT_JID.toString(), sender.chatJid},
                                                 {ID.toString(), sender.id}};

        execQuery(query,
                  QStringLiteral("SELECT ") + ID.toString() + QStringLiteral(" FROM " DB_TABLE_GROUP_CHAT_USERS)
                      + simpleWhereStatement(&sqlDriver(), keyValuePairs)
                      + QStringLiteral(" "
                                       "LIMIT 1"));

        if (!query.next()) {
            sender.status = GroupChatUser::Status::Left;
            addUser(sender);
        }
    });
}

QFuture<void> GroupChatUserDb::removeUsers(const QString &accountJid)
{
    return run([this, accountJid]() {
        _removeUsers(accountJid);
    });
}

void GroupChatUserDb::_removeUsers(const QString &accountJid)
{
    auto query = createQuery();

    QMap<QString, QVariant> keyValuePairs = {{ACCOUNT_JID.toString(), accountJid}};

    execQuery(query, QStringLiteral("DELETE FROM " DB_TABLE_GROUP_CHAT_USERS) + simpleWhereStatement(&sqlDriver(), keyValuePairs));
}

QFuture<void> GroupChatUserDb::removeUsers(const QString &accountJid, const QString &chatJid)
{
    return run([this, accountJid, chatJid]() {
        _removeUsers(accountJid, chatJid);
    });
}

void GroupChatUserDb::_removeUsers(const QString &accountJid, const QString &chatJid)
{
    auto query = createQuery();

    QMap<QString, QVariant> keyValuePairs = {
        {ACCOUNT_JID.toString(), accountJid},
        {CHAT_JID.toString(), chatJid},
    };

    execQuery(query, QStringLiteral("DELETE FROM " DB_TABLE_GROUP_CHAT_USERS) + simpleWhereStatement(&sqlDriver(), keyValuePairs));
}

void GroupChatUserDb::addUser(const GroupChatUser &user)
{
    const auto accountJid = user.accountJid;
    const auto chatJid = user.chatJid;

    insert(QStringLiteral(DB_TABLE_GROUP_CHAT_USERS),
           {
               {ACCOUNT_JID, accountJid},
               {CHAT_JID, chatJid},
               {ID, user.id},
               {JID, user.jid},
               {NAME, user.name},
               {STATUS, static_cast<int>(user.status)},
           });

    Q_EMIT userAdded(user);
    Q_EMIT userJidsChanged(accountJid, chatJid);
}

void GroupChatUserDb::updateUserById(const QString &accountJid,
                                     const QString &chatJid,
                                     const QString &userId,
                                     const std::function<void(GroupChatUser &)> &updateUser)
{
    updateUserByKeyValuePairs(updateUser, {{ACCOUNT_JID.toString(), accountJid}, {CHAT_JID.toString(), chatJid}, {ID.toString(), userId}});
}

void GroupChatUserDb::updateUserByJid(const QString &accountJid,
                                      const QString &chatJid,
                                      const QString &userJid,
                                      const std::function<void(GroupChatUser &)> &updateUser)
{
    updateUserByKeyValuePairs(updateUser, {{ACCOUNT_JID.toString(), accountJid}, {CHAT_JID.toString(), chatJid}, {JID.toString(), userJid}});
}

void GroupChatUserDb::updateUserByKeyValuePairs(const std::function<void(GroupChatUser &)> &updateUser, const QMap<QString, QVariant> &keyValuePairs)
{
    auto query = createQuery();
    query.setForwardOnly(true);

    // Load the user from the database.
    execQuery(query,
              QStringLiteral("SELECT * FROM " DB_TABLE_GROUP_CHAT_USERS) + simpleWhereStatement(&sqlDriver(), keyValuePairs)
                  + QStringLiteral(" "
                                   "LIMIT 1"));

    QList<GroupChatUser> users;
    parseUsersFromQuery(query, users);

    // Update the loaded user.
    if (!users.isEmpty()) {
        const auto oldUser = users.constFirst();
        auto newUser = oldUser;
        updateUser(newUser);

        // Replace the old user with the updated one if it has changed.
        if (oldUser != newUser) {
            // Create an SQL record containing the differences.
            QSqlRecord record = createUpdateRecord(users.constFirst(), newUser);

            execQuery(query,
                      sqlDriver().sqlStatement(QSqlDriver::UpdateStatement, QStringLiteral(DB_TABLE_GROUP_CHAT_USERS), record, false)
                          + simpleWhereStatement(&sqlDriver(), keyValuePairs));

            userUpdated(newUser);
        }
    }
}

void GroupChatUserDb::parseUsersFromQuery(QSqlQuery &query, QList<GroupChatUser> &users)
{
    QSqlRecord rec = query.record();

    int idxAccountJid = rec.indexOf(ACCOUNT_JID.toString());
    int idxChatJid = rec.indexOf(CHAT_JID.toString());
    int idxId = rec.indexOf(ID.toString());
    int idxJid = rec.indexOf(JID.toString());
    int idxName = rec.indexOf(NAME.toString());
    int idxStatus = rec.indexOf(STATUS.toString());

    while (query.next()) {
        GroupChatUser user;

        user.accountJid = query.value(idxAccountJid).toString();
        user.chatJid = query.value(idxChatJid).toString();
        user.id = query.value(idxId).toString();
        user.jid = query.value(idxJid).toString();
        user.name = query.value(idxName).toString();
        user.status = GroupChatUser::Status(query.value(idxStatus).toInt());

        users << user;
    }
}

QSqlRecord GroupChatUserDb::createUpdateRecord(const GroupChatUser &oldUser, const GroupChatUser &newUser)
{
    QSqlRecord rec;

    if (oldUser.accountJid != newUser.accountJid) {
        rec.append(createSqlField(ACCOUNT_JID.toString(), newUser.accountJid));
    }
    if (oldUser.chatJid != newUser.chatJid) {
        rec.append(createSqlField(CHAT_JID.toString(), newUser.chatJid));
    }
    if (oldUser.id != newUser.id) {
        rec.append(createSqlField(ID.toString(), newUser.id));
    }
    if (oldUser.jid != newUser.jid) {
        rec.append(createSqlField(JID.toString(), newUser.jid));
    }
    if (oldUser.name != newUser.name) {
        rec.append(createSqlField(NAME.toString(), newUser.name));
    }
    if (oldUser.status != newUser.status) {
        rec.append(createSqlField(STATUS.toString(), int(newUser.status)));
    }

    return rec;
}

void GroupChatUserDb::removeUser(const GroupChatUser &user)
{
    auto query = createQuery();

    const auto accountJid = user.accountJid;
    const auto chatJid = user.chatJid;

    // That variable must be used because users can have only an ID (participant in anonymous
    // group chat), only a JID (not joined but allowed user) or both (participant in normal
    // group chat).
    bool hasId = !user.id.isEmpty();

    QMap<QString, QVariant> keyValuePairs = {{ACCOUNT_JID.toString(), accountJid},
                                             {CHAT_JID.toString(), chatJid},
                                             {hasId ? ID.toString() : JID.toString(), hasId ? user.id : user.jid}};

    execQuery(query, QStringLiteral("DELETE FROM " DB_TABLE_GROUP_CHAT_USERS) + simpleWhereStatement(&sqlDriver(), keyValuePairs));

    Q_EMIT userRemoved(user);
    Q_EMIT userJidsChanged(accountJid, chatJid);
}

#include "moc_GroupChatUserDb.cpp"

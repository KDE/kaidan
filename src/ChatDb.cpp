// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChatDb.h"

// Qt
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
// Kaidan
#include "Algorithms.h"
#include "Globals.h"
#include "GroupChatUser.h"
#include "GroupChatUserDb.h"
#include "Message.h"
#include "MessageDb.h"
#include "RosterItem.h"
#include "SqlUtils.h"

Q_DECLARE_METATYPE(QXmppRosterIq::Item::SubscriptionType)

using namespace SqlUtils;

ChatDb *ChatDb::s_instance = nullptr;

ChatDb::ChatDb(QObject *parent)
    : DatabaseComponent(parent)
{
    Q_ASSERT(!ChatDb::s_instance);
    s_instance = this;
}

ChatDb::~ChatDb()
{
    s_instance = nullptr;
}

ChatDb *ChatDb::instance()
{
    return s_instance;
}

// The pure XMPP roster data ("roster" / "rosterGroups") is owned and written by RosterDb (the SQL
// QXmppRosterStorage backend). ChatDb only writes the chat-list / conversation data ("chats") and
// reads the roster data back via a LEFT JOIN to expose the fused RosterItem facade.
static QSqlRecord createChatsUpdateRecord(const RosterItem &oldItem, const RosterItem &newItem)
{
    QSqlRecord chatsRec;

    if (oldItem.groupChatName != newItem.groupChatName)
        chatsRec.append(createSqlField(QStringLiteral("groupChatName"), newItem.groupChatName));
    if (oldItem.groupChatDescription != newItem.groupChatDescription)
        chatsRec.append(createSqlField(QStringLiteral("groupChatDescription"), newItem.groupChatDescription));
    if (oldItem.groupChatFlags != newItem.groupChatFlags)
        chatsRec.append(createSqlField(QStringLiteral("groupChatFlags"), static_cast<int>(newItem.groupChatFlags)));
    if (oldItem.encryption != newItem.encryption)
        chatsRec.append(createSqlField(QStringLiteral("encryption"), newItem.encryption));
    if (oldItem.lastReadOwnMessageId != newItem.lastReadOwnMessageId)
        chatsRec.append(createSqlField(QStringLiteral("lastReadOwnMessageId"), newItem.lastReadOwnMessageId));
    if (oldItem.lastReadContactMessageId != newItem.lastReadContactMessageId)
        chatsRec.append(createSqlField(QStringLiteral("lastReadContactMessageId"),
                                       newItem.lastReadContactMessageId.isEmpty() ? QVariant{} : newItem.lastReadContactMessageId));
    if (oldItem.latestGroupChatMessageStanzaId != newItem.latestGroupChatMessageStanzaId)
        chatsRec.append(createSqlField(QStringLiteral("latestGroupChatMessageStanzaId"), newItem.latestGroupChatMessageStanzaId));
    if (oldItem.latestGroupChatMessageStanzaTimestamp != newItem.latestGroupChatMessageStanzaTimestamp)
        chatsRec.append(createSqlField(QStringLiteral("latestGroupChatMessageStanzaTimestamp"), newItem.latestGroupChatMessageStanzaTimestamp));
    if (oldItem.readMarkerPending != newItem.readMarkerPending)
        chatsRec.append(createSqlField(QStringLiteral("readMarkerPending"), newItem.readMarkerPending));
    if (oldItem.pinningPosition != newItem.pinningPosition)
        chatsRec.append(createSqlField(QStringLiteral("pinningPosition"), newItem.pinningPosition));
    if (oldItem.chatStateSendingEnabled != newItem.chatStateSendingEnabled)
        chatsRec.append(createSqlField(QStringLiteral("chatStateSendingEnabled"), newItem.chatStateSendingEnabled));
    if (oldItem.readMarkerSendingEnabled != newItem.readMarkerSendingEnabled)
        chatsRec.append(createSqlField(QStringLiteral("readMarkerSendingEnabled"), newItem.readMarkerSendingEnabled));
    if (oldItem.notificationRule != newItem.notificationRule)
        chatsRec.append(createSqlField(QStringLiteral("notificationRule"), static_cast<int>(newItem.notificationRule)));
    if (oldItem.automaticMediaDownloadsRule != newItem.automaticMediaDownloadsRule)
        chatsRec.append(createSqlField(QStringLiteral("automaticMediaDownloadsRule"), static_cast<int>(newItem.automaticMediaDownloadsRule)));

    return chatsRec;
}

QFuture<QList<RosterItem>> ChatDb::fetchItems()
{
    return run([this]() {
        return _fetchItems();
    });
}

QFuture<void> ChatDb::addItem(RosterItem item)
{
    return run([this, item]() mutable {
        fetchLastMessage(item);
        _addItem(item);
    });
}

QFuture<void> ChatDb::updateItem(const QString &accountJid, const QString &jid, const std::function<void(RosterItem &)> &updateItem)
{
    return run([this, accountJid, jid, updateItem]() {
        _updateItem(accountJid, jid, updateItem);
    });
}

QFuture<void> ChatDb::refreshItem(const QString &accountJid, const QString &jid)
{
    return run([this, accountJid, jid]() {
        _refreshItem(accountJid, jid);
    });
}

QFuture<void> ChatDb::replaceItems(const QString &accountJid, const QList<RosterItem> &items)
{
    return run([this, accountJid, items]() {
        // Load the current chats (the chat list is the primary set), joined to their roster data.
        // The roster table itself is owned by RosterDb; here we reconcile the chats rows against the
        // new server roster. Every chat currently corresponds to a roster item (contact or MIX
        // channel).
        // TODO: Once MUC group chats are stored as bookmarks (chats rows without a roster row), skip
        // those here so they are not removed.
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
				SELECT *
				FROM )" DB_TABLE_CHATS R"( LEFT JOIN )" DB_TABLE_ROSTER R"( USING (accountJid, jid)
				WHERE accountJid = :accountJid
			)"),
                  {{u":accountJid", accountJid}});

        auto oldItems = parseItemsFromQuery(query);

        for (auto &oldItem : oldItems) {
            fetchGroups(oldItem);
        }

        transaction();

        QList<QString> newJids = transform(items, [](const RosterItem &rosterItem) {
            return rosterItem.jid;
        });

        for (const auto &oldItem : std::as_const(oldItems)) {
            const auto jid = oldItem.jid;

            // We will remove the already existing JIDs, so we get a set of JIDs that
            // are completely new.
            //
            // By calling remove(), we also find out whether the JID is already
            // existing or not.
            if (newJids.removeOne(jid)) {
                // item is also included in newJids -> refresh the fused view (the roster data itself
                // is owned and was already updated by RosterDb)
                _refreshItem(accountJid, jid);
            } else {
                // item is not included in newJids -> delete
                _removeItem(accountJid, jid);
            }
        }

        // now add the completely new JIDs
        for (const QString &jid : newJids) {
            auto itr = std::ranges::find_if(items, [jid](const RosterItem &rosterItem) {
                return rosterItem.jid == jid;
            });
            _addItem(*itr);
        }

        commit();
        Q_EMIT itemsReplaced(accountJid);
    });
}

QFuture<void> ChatDb::removeItem(const QString &accountJid, const QString &jid)
{
    return run([this, accountJid, jid]() {
        _removeItem(accountJid, jid);
    });
}

QFuture<void> ChatDb::removeItems(const QString &accountJid)
{
    return run([this, accountJid]() {
        auto query = createQuery();

        execQuery(query,
                  QStringLiteral("DELETE FROM " DB_TABLE_CHATS " "
                                 "WHERE accountJid = :accountJid"),
                  {{u":accountJid", accountJid}});

        GroupChatUserDb::instance()->_removeUsers(accountJid);

        itemsRemoved(accountJid);
    });
}

QList<RosterItem> ChatDb::_fetchItems()
{
    auto items = fetchBasicItems();

    for (auto &item : items) {
        fetchGroups(item);
        fetchLastMessage(item, items);
        fetchUnreadMessageCount(item);
        fetchMarkedMessageCount(item);
    }

    return items;
}

QList<RosterItem> ChatDb::fetchBasicItems()
{
    auto query = createQuery();
    // The chats table is the primary set: every chat is listed, joining its roster data if it is
    // also an XMPP roster entry (future MUC group chats are chats without a roster entry).
    execQuery(query, QStringLiteral("SELECT * FROM " DB_TABLE_CHATS " LEFT JOIN " DB_TABLE_ROSTER " USING (accountJid, jid)"));
    return parseItemsFromQuery(query);
}

void ChatDb::fetchGroups(RosterItem &item)
{
    enum {
        Group,
    };

    auto query = createQuery();

    execQuery(query,
              QStringLiteral(R"(
				SELECT name
				FROM rosterGroups
				WHERE accountJid = :accountJid AND chatJid = :jid
			)"),
              {
                  {u":accountJid", item.accountJid},
                  {u":jid", item.jid},
              });

    // Iterate over all found groups.
    while (query.next()) {
        auto &groups = item.groups;
        groups.append(query.value(Group).toString());
    }
}

void ChatDb::fetchLastMessage(RosterItem &item)
{
    fetchLastMessage(item, fetchBasicItems());
}

void ChatDb::fetchLastMessage(RosterItem &item, const QList<RosterItem> &allItems)
{
    const auto accountJid = item.accountJid;
    const auto jid = item.jid;

    auto lastMessage = MessageDb::instance()->_fetchLastMessage(accountJid, jid);
    item.lastMessageDateTime = lastMessage.timestamp;
    item.lastMessage = lastMessage.previewText();
    item.lastMessageDeliveryState = lastMessage.deliveryState;
    item.lastMessageIsOwn = lastMessage.isOwn;

    if (item.isGroupChat()) {
        if (const auto lastMessageSender = GroupChatUserDb::instance()->_user(accountJid, jid, lastMessage.groupChatSenderId)) {
            if (const auto lastMessageSenderJid = lastMessageSender->jid; lastMessageSenderJid.isEmpty()) {
                item.lastMessageGroupChatSenderName = lastMessageSender->displayName();
            } else {
                const auto itr = std::ranges::find_if(allItems, [accountJid, lastMessageSenderJid](const RosterItem &rosterItem) {
                    return rosterItem.accountJid == accountJid && rosterItem.jid == lastMessageSenderJid;
                });

                if (itr == allItems.cend()) {
                    item.lastMessageGroupChatSenderName = lastMessageSender->displayName();
                } else {
                    item.lastMessageGroupChatSenderName = itr->displayName();
                }
            }
        } else {
            item.lastMessageGroupChatSenderName = lastMessage.groupChatSenderId;
        }
    }
}

void ChatDb::fetchUnreadMessageCount(RosterItem &item)
{
    item.unreadMessageCount = MessageDb::instance()->_latestContactMessageCount(item.accountJid, item.jid, item.lastReadContactMessageId);
}

void ChatDb::fetchMarkedMessageCount(RosterItem &item)
{
    item.markedMessageCount = MessageDb::instance()->_markedMessageCount(item.accountJid, item.jid);
}

void ChatDb::_addItem(RosterItem item)
{
    fetchUnreadMessageCount(item);
    fetchMarkedMessageCount(item);
    Q_EMIT itemAdded(item);

    insert(QString::fromLatin1(DB_TABLE_CHATS),
           {
               {u"accountJid", item.accountJid},
               {u"jid", item.jid},
               {u"groupChatName", item.groupChatName},
               {u"groupChatDescription", item.groupChatDescription},
               {u"groupChatFlags", static_cast<int>(item.groupChatFlags)},
               {u"encryption", item.encryption},
               {u"lastReadOwnMessageId", QVariant{}},
               {u"lastReadContactMessageId", QVariant{}},
               {u"latestGroupChatMessageStanzaId", QVariant{}},
               {u"latestGroupChatMessageStanzaTimestamp", QVariant{}},
               {u"readMarkerPending", item.readMarkerPending},
               {u"pinningPosition", item.pinningPosition},
               {u"chatStateSendingEnabled", item.chatStateSendingEnabled},
               {u"readMarkerSendingEnabled", item.readMarkerSendingEnabled},
               {u"notificationRule", static_cast<int>(item.notificationRule)},
               {u"automaticMediaDownloadsRule", static_cast<int>(item.automaticMediaDownloadsRule)},
           });
}

void ChatDb::_updateItem(const QString &accountJid, const QString &jid, const std::function<void(RosterItem &)> &updateItem)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
			SELECT *
			FROM )" DB_TABLE_CHATS R"( LEFT JOIN )" DB_TABLE_ROSTER R"( USING (accountJid, jid)
			WHERE accountJid = :accountJid AND jid = :jid
			LIMIT 1
		)"),
              {
                  {u":accountJid", accountJid},
                  {u":jid", jid},
              });

    if (query.first()) {
        auto oldItem = parseItemFromQuery(query);

        fetchGroups(oldItem);
        fetchLastMessage(oldItem);
        fetchUnreadMessageCount(oldItem);
        fetchMarkedMessageCount(oldItem);

        auto newItem = oldItem;
        updateItem(newItem);

        // Replace the old item's values with the updated ones if the item has changed.
        if (oldItem != newItem) {
            fetchUnreadMessageCount(newItem);
            fetchMarkedMessageCount(newItem);
            itemUpdated(newItem);

            // Persist only the chat-list differences; the roster data is owned by RosterDb.
            auto chatsRecord = createChatsUpdateRecord(oldItem, newItem);
            if (!chatsRecord.isEmpty()) {
                updateItemByRecord(QString::fromLatin1(DB_TABLE_CHATS), accountJid, jid, chatsRecord);
            }
        }
    }
}

void ChatDb::_refreshItem(const QString &accountJid, const QString &jid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
			SELECT *
			FROM )" DB_TABLE_CHATS R"( LEFT JOIN )" DB_TABLE_ROSTER R"( USING (accountJid, jid)
			WHERE accountJid = :accountJid AND jid = :jid
			LIMIT 1
		)"),
              {
                  {u":accountJid", accountJid},
                  {u":jid", jid},
              });

    if (query.first()) {
        auto item = parseItemFromQuery(query);

        fetchGroups(item);
        fetchLastMessage(item);
        fetchUnreadMessageCount(item);
        fetchMarkedMessageCount(item);

        Q_EMIT itemUpdated(item);
    }
}

void ChatDb::_removeItem(const QString &accountJid, const QString &jid)
{
    Q_EMIT itemRemoved(accountJid, jid);

    auto query = createQuery();

    execQuery(query,
              QStringLiteral("DELETE FROM " DB_TABLE_CHATS " "
                             "WHERE accountJid = :accountJid AND jid = :jid"),
              {{u":accountJid", accountJid}, {u":jid", jid}});

    GroupChatUserDb::instance()->_removeUsers(accountJid, jid);
}

QList<RosterItem> ChatDb::parseItemsFromQuery(QSqlQuery &query)
{
    QList<RosterItem> items;

    while (query.next()) {
        items.append(parseItemFromQuery(query));
    }

    return items;
}

RosterItem ChatDb::parseItemFromQuery(QSqlQuery &query)
{
    QSqlRecord rec = query.record();

    int idxAccountJid = rec.indexOf(QStringLiteral("accountJid"));
    int idxJid = rec.indexOf(QStringLiteral("jid"));
    int idxName = rec.indexOf(QStringLiteral("name"));
    int idxSubscription = rec.indexOf(QStringLiteral("subscription"));
    int idxGroupChatParticipantId = rec.indexOf(QStringLiteral("groupChatParticipantId"));
    int idxGroupChatName = rec.indexOf(QStringLiteral("groupChatName"));
    int idxGroupChatDescription = rec.indexOf(QStringLiteral("groupChatDescription"));
    int idxGroupChatFlags = rec.indexOf(QStringLiteral("groupChatFlags"));
    int idxEncryption = rec.indexOf(QStringLiteral("encryption"));
    int idxLastReadOwnMessageId = rec.indexOf(QStringLiteral("lastReadOwnMessageId"));
    int idxLastReadContactMessageId = rec.indexOf(QStringLiteral("lastReadContactMessageId"));
    int idxLatestGroupChatMessageStanzaId = rec.indexOf(QStringLiteral("latestGroupChatMessageStanzaId"));
    int idxLatestGroupChatMessageStanzaTimestamp = rec.indexOf(QStringLiteral("latestGroupChatMessageStanzaTimestamp"));
    int idxReadMarkerPending = rec.indexOf(QStringLiteral("readMarkerPending"));
    int idxPinningPosition = rec.indexOf(QStringLiteral("pinningPosition"));
    int idxChateStateSendingEnabled = rec.indexOf(QStringLiteral("chatStateSendingEnabled"));
    int idxReadMarkerSendingEnabled = rec.indexOf(QStringLiteral("readMarkerSendingEnabled"));
    int idxNotificationRule = rec.indexOf(QStringLiteral("notificationRule"));
    int idxAutomaticMediaDownloadsRule = rec.indexOf(QStringLiteral("automaticMediaDownloadsRule"));

    RosterItem item;

    item.accountJid = query.value(idxAccountJid).toString();
    item.jid = query.value(idxJid).toString();
    item.name = query.value(idxName).toString();
    item.subscription = query.value(idxSubscription).value<QXmppRosterIq::Item::SubscriptionType>();
    item.groupChatParticipantId = query.value(idxGroupChatParticipantId).toString();
    item.groupChatName = query.value(idxGroupChatName).toString();
    item.groupChatDescription = query.value(idxGroupChatDescription).toString();
    item.groupChatFlags = query.value(idxGroupChatFlags).value<RosterItem::GroupChatFlags>();
    item.groupChatFlags = static_cast<RosterItem::GroupChatFlags>(query.value(idxGroupChatFlags).value<RosterItem::GroupChatFlags::Int>());
    item.encryption = query.value(idxEncryption).value<Encryption::Enum>();
    item.lastReadOwnMessageId = query.value(idxLastReadOwnMessageId).toString();
    item.lastReadContactMessageId = query.value(idxLastReadContactMessageId).toString();
    item.latestGroupChatMessageStanzaId = query.value(idxLatestGroupChatMessageStanzaId).toString();
    item.latestGroupChatMessageStanzaTimestamp = query.value(idxLatestGroupChatMessageStanzaTimestamp).toDateTime();
    item.readMarkerPending = query.value(idxReadMarkerPending).toBool();
    item.pinningPosition = query.value(idxPinningPosition).toInt();
    item.chatStateSendingEnabled = query.value(idxChateStateSendingEnabled).toBool();
    item.readMarkerSendingEnabled = query.value(idxReadMarkerSendingEnabled).toBool();
    item.notificationRule = query.value(idxNotificationRule).value<RosterItem::NotificationRule>();
    item.automaticMediaDownloadsRule = query.value(idxAutomaticMediaDownloadsRule).value<RosterItem::AutomaticMediaDownloadsRule>();

    return item;
}

void ChatDb::updateItemByRecord(const QString &table, const QString &accountJid, const QString &jid, const QSqlRecord &record)
{
    auto query = createQuery();
    auto &driver = sqlDriver();

    QMap<QString, QVariant> keyValuePairs = {{QStringLiteral("accountJid"), accountJid}, {QStringLiteral("jid"), jid}};

    execQuery(query, driver.sqlStatement(QSqlDriver::UpdateStatement, table, record, false) + simpleWhereStatement(&driver, keyValuePairs));
}

#include "moc_ChatDb.cpp"

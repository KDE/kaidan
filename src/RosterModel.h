// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <optional>
// Qt
#include <QAbstractListModel>
#include <QFuture>
// Kaidan
#include "RosterItem.h"

class Account;
class Message;
enum class MessageOrigin : quint8;

class RosterModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QStringList groups READ groups NOTIFY groupsChanged)

public:
    enum RosterItemRoles {
        AccountRole,
        JidRole,
        NameRole,
        GroupsRole,
        IsNotesChatRole,
        IsProviderChatRole,
        IsGroupChatRole,
        IsPublicGroupChatRole,
        IsDeletedGroupChatRole,
        LastMessageDateTimeRole,
        UnreadMessageCountRole,
        MarkedMessageCountRole,
        LastMessageRole,
        LastMessageIsDraftRole,
        LastMessageIsOwnRole,
        LastMessageGroupChatSenderNameRole,
        PinnedRole,
        SelectedRole,
        EffectiveNotificationRuleRole,
    };
    Q_ENUM(RosterItemRoles)

    static RosterModel *instance();

    explicit RosterModel(QObject *parent = nullptr);
    ~RosterModel() override;

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

    /**
     * Returns whether this model has a roster item with the passed properties.
     *
     * @param jid JID of the roster item
     *
     * @return true if a roster item with the passed properties exists, otherwise false
     */
    Q_INVOKABLE bool hasItem(const QString &accountJid, const QString &jid) const;

    Q_SIGNAL void itemsFetched(const QList<RosterItem> &items);
    Q_SIGNAL void itemAdded(const RosterItem &item);
    Q_SIGNAL void itemRemoved(const QString &accountJid, const QString &jid);
    Q_SIGNAL void itemsRemoved(const QString &accountJid);

    /**
     * Returns the roster groups of all roster items.
     *
     * @return all roster groups
     */
    QStringList groups() const;
    Q_SIGNAL void groupsChanged();

    std::optional<Encryption::Enum> itemEncryption(const QString &accountJid, const QString &jid) const;
    void setItemEncryption(const QString &accountJid, const QString &jid, Encryption::Enum encryption);
    Q_INVOKABLE void setItemEncryption(const QString &accountJid, Encryption::Enum encryption);

    QString lastReadOwnMessageId(const QString &accountJid, const QString &jid) const;
    QString lastReadContactMessageId(const QString &accountJid, const QString &jid) const;

    /**
     * Searches for the roster item with a given JID.
     */
    std::optional<RosterItem> item(const QString &accountJid, const QString &jid) const;

    const QList<RosterItem> &items() const;
    const QList<RosterItem> items(const QString &accountJid) const;

    Q_INVOKABLE void pinItem(const QString &accountJid, const QString &jid);
    Q_INVOKABLE void unpinItem(const QString &accountJid, const QString &jid);
    Q_INVOKABLE void reorderPinnedItem(int oldIndex, int newIndex);

    Q_INVOKABLE void toggleSelected(const QString &accountJid, const QString &jid);
    Q_INVOKABLE void resetSelected();

private:
    void handleItemsFetched(const QList<RosterItem> &items);

    void addItem(const RosterItem &item);
    void updateItem(const RosterItem &item);
    void removeItem(const QString &accountJid, const QString &jid);
    void removeItems(const QString &accountJid);

    void initializeNotificationRuleUpdates();
    void initializeNotificationRuleUpdates(Account *account);
    void handleAccountNotificationRuleChanged();

    void handleMessageAdded(const Message &message, MessageOrigin origin);
    void handleMessageUpdated(const Message &message);
    void handleMessageRemoved(const Message &newLastMessage);

    void handleDraftMessageAdded(const Message &message);
    void handleDraftMessageUpdated(const Message &message);
    void handleDraftMessageRemoved(const Message &newLastMessage);

    QFuture<QList<int>> updateLastMessage(QList<RosterItem>::Iterator &itr, const Message &message, bool onlyUpdateIfNewerOrAtSameAge = true);

    void updateOnMessageChange(QList<RosterItem>::Iterator &itr, const QList<int> &changedRoles);
    void updateOnDraftMessageChange(QList<RosterItem>::Iterator &itr);

    int informAboutChangedData(QList<RosterItem>::Iterator &itr, const QList<int> &changedRoles);

    void insertItem(int index, const RosterItem &item);
    void updateItemPosition(int currentIndex);
    int positionToAdd(const RosterItem &item);
    int positionToMove(int currentIndex);

    QString formatLastMessageDateTime(const QDateTime &lastMessageDateTime) const;

    QList<RosterItem> m_items;

    static RosterModel *s_instance;
};

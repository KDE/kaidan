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

class Message;
enum class MessageOrigin : quint8;

class RosterModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QStringList accountJids READ accountJids NOTIFY accountJidsChanged)
    Q_PROPERTY(QStringList groups READ groups NOTIFY groupsChanged)

public:
    enum RosterItemRoles {
        AccountJidRole,
        JidRole,
        NameRole,
        GroupsRole,
        IsGroupChatRole,
        IsPublicGroupChatRole,
        IsDeletedGroupChatRole,
        LastMessageDateTimeRole,
        UnreadMessagesRole,
        LastMessageRole,
        LastMessageIsDraftRole,
        LastMessageIsOwnRole,
        LastMessageGroupChatSenderNameRole,
        PinnedRole,
        SelectedRole,
        NotificationRuleRole,
    };
    Q_ENUM(RosterItemRoles)

    /**
     * Result for adding a contact by an XMPP URI specifying how the URI is used
     */
    enum AddContactByUriResult {
        AddingContact, ///< The contact is being added to the roster.
        ContactExists, ///< The contact is already in the roster.
        InvalidUri ///< The URI cannot be used for contact addition.
    };
    Q_ENUM(AddContactByUriResult)

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
    Q_INVOKABLE bool hasItem(const QString &jid) const;

    Q_SIGNAL void itemAdded(const QString &accountJid, const QString &jid);
    Q_SIGNAL void itemRemoved(const QString &accountJid, const QString &jid);

    /**
     * Returns the account JIDs of all roster items.
     *
     * @return all account JIDs
     */
    QStringList accountJids() const;
    Q_SIGNAL void accountJidsChanged();

    /**
     * Returns the roster groups of all roster items.
     *
     * @return all roster groups
     */
    QStringList groups() const;
    Q_SIGNAL void groupsChanged();

    Q_INVOKABLE void updateGroup(const QString &oldGroup, const QString &newGroup);
    Q_INVOKABLE void removeGroup(const QString &group);

    /**
     * Returns whether an account's presence is subscribed by a roster item.
     *
     * @param accountJid JID of the account whose roster item's presence subcription is requested
     * @param jid JID of the roster item
     *
     * @return whether the presence is subscribed by the item
     */
    bool isPresenceSubscribedByItem(const QString &accountJid, const QString &jid) const;

    std::optional<Encryption::Enum> itemEncryption(const QString &accountJid, const QString &jid) const;
    void setItemEncryption(const QString &accountJid, const QString &jid, Encryption::Enum encryption);
    Q_INVOKABLE void setItemEncryption(const QString &accountJid, Encryption::Enum encryption);

    /**
     * Adds a contact (bare JID) by a given XMPP URI (e.g., from a scanned QR
     * code) such as "xmpp:user@example.org".
     *
     * @param uriString XMPP URI string that contains only a JID
     */
    Q_INVOKABLE RosterModel::AddContactByUriResult addContactByUri(const QString &accountJid, const QString &uriString);

    QString lastReadOwnMessageId(const QString &accountJid, const QString &jid) const;
    QString lastReadContactMessageId(const QString &accountJid, const QString &jid) const;

    /**
     * Sends read markers for all roster items that have unsent (pending) ones.
     *
     * This method must only be called while being logged in.
     * Otherwise, the pending states of the read markers would be reset even if the pending read
     * markers could not be sent.
     *
     * @param accountJid bare JID of the user's account
     */
    void sendPendingReadMarkers(const QString &accountJid);

    /**
     * Searches for the roster item with a given JID.
     */
    std::optional<RosterItem> findItem(const QString &jid) const;

    const QList<RosterItem> &items() const;

    Q_INVOKABLE void pinItem(const QString &accountJid, const QString &jid);
    Q_INVOKABLE void unpinItem(const QString &accountJid, const QString &jid);
    Q_INVOKABLE void reorderPinnedItem(const QString &accountJid, const QString &jid, int oldIndex, int newIndex);

    Q_INVOKABLE void toggleSelected(const QString &accountJid, const QString &jid);
    Q_INVOKABLE void resetSelected();

    Q_INVOKABLE void setChatStateSendingEnabled(const QString &accountJid, const QString &jid, bool chatStateSendingEnabled);
    Q_INVOKABLE void setReadMarkerSendingEnabled(const QString &accountJid, const QString &jid, bool readMarkerSendingEnabled);
    Q_INVOKABLE void setNotificationRule(const QString &accountJid, const QString &jid, RosterItem::NotificationRule notificationRule);
    Q_INVOKABLE void setAutomaticMediaDownloadsRule(const QString &accountJid, const QString &jid, RosterItem::AutomaticMediaDownloadsRule rule);

private:
    void handleItemsFetched(const QList<RosterItem> &items);

    void addItem(const RosterItem &item);
    void updateItem(const RosterItem &item);
    void removeItem(const QString &accountJid, const QString &jid);
    void removeItems(const QString &accountJid);

    void handleAccountChanged();

    void handleMessageAdded(const Message &message, MessageOrigin origin);
    void handleMessageUpdated(const Message &message);
    void handleDraftMessageAdded(const Message &message);
    void handleDraftMessageUpdated(const Message &message);
    void handleDraftMessageRemoved(const Message &newLastMessage);
    void handleMessageRemoved(const Message &newLastMessage);

    QFuture<QList<int>> updateLastMessage(QList<RosterItem>::Iterator &itr, const Message &message, bool onlyUpdateIfNewerOrAtSameAge = true);

    void updateOnDraftMessageChanged(QList<RosterItem>::Iterator &itr);
    int informAboutChangedData(QList<RosterItem>::Iterator &itr, const QList<int> &changedRoles);
    void insertItem(int index, const RosterItem &item);
    void updateItemPosition(int currentIndex);
    int positionToAdd(const RosterItem &item);
    int positionToMove(int currentIndex);

    QString formatLastMessageDateTime(const QDateTime &lastMessageDateTime) const;
    QString determineGroupChatSenderName(const Message &message) const;

    QList<RosterItem> m_items;

    static RosterModel *s_instance;
};

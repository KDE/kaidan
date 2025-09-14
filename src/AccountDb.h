// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "Account.h"
#include "DatabaseComponent.h"

class AccountDb : public DatabaseComponent
{
    Q_OBJECT

    friend class Database;

public:
    static AccountDb *instance();

    explicit AccountDb(QObject *parent = nullptr);
    ~AccountDb();

    QFuture<void> addAccount(const AccountSettings::Data &account);
    Q_SIGNAL void accountAdded(const AccountSettings::Data &account);

    QFuture<QList<AccountSettings::Data>> accounts();

    QFuture<void> updateAccount(const QString &jid, const std::function<void(AccountSettings::Data &)> &updateAccount);

    QFuture<void> updateCredentials(const QString &jid, const QXmppCredentials &credentials);
    QFuture<void> updateEnabled(const QString &jid, bool enabled);
    QFuture<void> updateHttpUploadLimit(const QString &jid, qint64 httpUploadLimit);
    QFuture<void> updatePasswordVisibility(const QString &jid, AccountSettings::PasswordVisibility passwordVisibility);
    QFuture<void> updateEncryption(const QString &jid, Encryption::Enum encryption);
    QFuture<void> updateAutomaticMediaDownloadsRule(const QString &jid, AccountSettings::AutomaticMediaDownloadsRule automaticMediaDownloadsRule);
    QFuture<void> updateContactNotificationRule(const QString &jid, AccountSettings::ContactNotificationRule contactNotificationRule);
    QFuture<void> updateGroupChatNotificationRule(const QString &jid, AccountSettings::GroupChatNotificationRule groupChatNotificationRule);
    QFuture<void> updateGeoLocationMapPreviewEnabled(const QString &jid, bool geoLocationMapPreviewEnabled);
    QFuture<void> updateGeoLocationMapService(const QString &jid, AccountSettings::GeoLocationMapService geoLocationMapService);

    /**
     * Fetches the stanza ID of the latest locally stored (existing or removed) message.
     */
    QFuture<QString> fetchLatestMessageStanzaId(const QString &jid);

    QFuture<void> removeAccount(const QString &jid);
    Q_SIGNAL void accountRemoved(const QString &jid);

private:
    static void parseAccountsFromQuery(QSqlQuery &query, QList<AccountSettings::Data> &accounts);
    static QSqlRecord createUpdateRecord(const AccountSettings::Data &oldAccount, const AccountSettings::Data &newAccount);
    void updateAccountByRecord(const QString &jid, const QSqlRecord &record);

    QFuture<void> updateField(const QString &jid, const QString &column, const QVariant &value);

    static AccountDb *s_instance;
};

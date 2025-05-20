// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>
// Kaidan
#include "PresenceCache.h"

class QXmppVersionIq;
class VersionController;

class UserDevicesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(VersionController *versionController MEMBER m_versionController WRITE setVersionController)
    Q_PROPERTY(PresenceCache *presenceCache MEMBER m_presenceCache WRITE setPresenceCache)
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)

public:
    enum Roles {
        Resource = Qt::UserRole + 1,
        Name,
        Version,
        OS,
    };

    explicit UserDevicesModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;

    void setVersionController(VersionController *versionController);
    void setPresenceCache(PresenceCache *presenceCache);

    QString jid() const;
    void setJid(const QString &jid);
    Q_SIGNAL void jidChanged();

private:
    void handleClientVersionReceived(const QXmppVersionIq &versionIq);
    void handlePresenceChanged(PresenceCache::ChangeType type, const QString &jid, const QString &resource);
    void handlePresencesCleared();

    struct DeviceInfo {
        explicit DeviceInfo(const QString &resource);
        explicit DeviceInfo(const QXmppVersionIq &);

        QString resource;
        QString name;
        QString version;
        QString os;
    };

    VersionController *m_versionController;
    PresenceCache *m_presenceCache;
    QString m_jid;

    QList<DeviceInfo> m_devices;
};

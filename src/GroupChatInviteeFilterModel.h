// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QSortFilterProxyModel>

class GroupChatInviteeFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QString accountJid MEMBER m_accountJid WRITE setAccountJid)
    Q_PROPERTY(QList<QString> groupChatUserJids MEMBER m_groupChatUserJids WRITE setGroupChatUserJids)

public:
    explicit GroupChatInviteeFilterModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    void setAccountJid(const QString &accountJid);
    void setGroupChatUserJids(const QList<QString> &groupChatUserJids);

    QString m_accountJid;
    QList<QString> m_groupChatUserJids;
};

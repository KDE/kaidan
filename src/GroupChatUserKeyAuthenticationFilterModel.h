// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QSortFilterProxyModel>

class EncryptionController;

class GroupChatUserKeyAuthenticationFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit GroupChatUserKeyAuthenticationFilterModel(QObject *parent = nullptr);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    void setUp();
    void handleDevicesChanged(const QList<QString> &jids);
    void updateJids();

    QList<QString> m_jids;
    EncryptionController *m_encryptionController;
};

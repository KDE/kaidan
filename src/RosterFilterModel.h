// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QSortFilterProxyModel>

class PresenceCache;

class RosterFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(Types displayedTypes READ displayedTypes NOTIFY displayedTypesChanged)
    Q_PROPERTY(QList<QString> selectedAccountJids READ selectedAccountJids WRITE setSelectedAccountJids NOTIFY selectedAccountJidsChanged)
    Q_PROPERTY(QList<QString> selectedGroups READ selectedGroups WRITE setSelectedGroups NOTIFY selectedGroupsChanged)

public:
    enum class Type {
        AvailableContact = 1 << 0,
        UnavailableContact = 1 << 1,
        PrivateGroupChat = 1 << 2,
        PublicGroupChat = 1 << 3,
    };
    Q_ENUM(Type)
    Q_DECLARE_FLAGS(Types, Type)
    Q_FLAGS(Types)

    explicit RosterFilterModel(QObject *parent = nullptr);

    Q_INVOKABLE void addDisplayedType(RosterFilterModel::Type type);
    Q_INVOKABLE void removeDisplayedType(RosterFilterModel::Type type);
    Q_INVOKABLE void resetDisplayedTypes();
    Types displayedTypes() const;
    Q_SIGNAL void displayedTypesChanged();

    void setSelectedAccountJids(const QList<QString> &selectedAccountJids);
    QList<QString> selectedAccountJids() const;
    Q_SIGNAL void selectedAccountJidsChanged();

    void setSelectedGroups(const QList<QString> &selectedGroups);
    QList<QString> selectedGroups() const;
    Q_SIGNAL void selectedGroupsChanged();

    Q_INVOKABLE void reorderPinnedItem(int oldIndex, int newIndex);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    void updateSelectedAccountJids();
    void updateSelectedGroups();

    Types m_displayedTypes;
    QList<QString> m_selectedAccountJids;
    QList<QString> m_selectedGroups;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RosterFilterModel::Types)

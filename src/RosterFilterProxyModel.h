// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>

class RosterFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	Q_PROPERTY(Types displayedTypes READ displayedTypes NOTIFY displayedTypesChanged)
	Q_PROPERTY(QVector<QString> selectedAccountJids READ selectedAccountJids WRITE setSelectedAccountJids NOTIFY selectedAccountJidsChanged)
	Q_PROPERTY(QVector<QString> selectedGroups READ selectedGroups WRITE setSelectedGroups NOTIFY selectedGroupsChanged)
	Q_PROPERTY(bool groupChatsExcluded READ groupChatsExcluded WRITE setGroupChatsExcluded NOTIFY groupChatsExcludedChanged)
	Q_PROPERTY(bool groupChatUsersExcluded READ groupChatUsersExcluded WRITE setGroupChatUsersExcluded NOTIFY groupChatUsersExcludedChanged)

public:
	enum class Type {
		UnavailableContact = 1 << 0,
		AvailableContact = 1 << 1,
		PrivateGroupChat = 1 << 2,
		PublicGroupChat = 1 << 3,
	};
	Q_ENUM(Type)
	Q_DECLARE_FLAGS(Types, Type)
	Q_FLAGS(Types)

	RosterFilterProxyModel(QObject *parent = nullptr);

	Q_INVOKABLE void addDisplayedType(RosterFilterProxyModel::Type type);
	Q_INVOKABLE void removeDisplayedType(RosterFilterProxyModel::Type type);
	Q_INVOKABLE void resetDisplayedTypes();
	Types displayedTypes() const;
	Q_SIGNAL void displayedTypesChanged();

	void setSelectedAccountJids(const QVector<QString> &selectedAccountJids);
	QVector<QString> selectedAccountJids() const;
	Q_SIGNAL void selectedAccountJidsChanged();

	void setSelectedGroups(const QVector<QString> &selectedGroups);
	QVector<QString> selectedGroups() const;
	Q_SIGNAL void selectedGroupsChanged();

	void setGroupChatsExcluded(bool groupChatsExcluded);
	bool groupChatsExcluded();
	Q_SIGNAL void groupChatsExcludedChanged();

	void setGroupChatUsersExcluded(bool groupChatUsersExcluded);
	bool groupChatUsersExcluded();
	Q_SIGNAL void groupChatUsersExcludedChanged();

	Q_INVOKABLE void reorderPinnedItem(const QString &accountJid, const QString &jid, int oldIndex, int newIndex);

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
	void updateGroupChatUserJids();

	Types m_displayedTypes;
	QVector<QString> m_selectedAccountJids;
	QVector<QString> m_selectedGroups;
	QVector<QString> m_groupChatUserJids;
	bool m_groupChatsExcluded = false;
	bool m_groupChatUsersExcluded = false;
};

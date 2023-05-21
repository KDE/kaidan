// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PublicGroupChatModel.h"

PublicGroupChatModel::PublicGroupChatModel(QObject *parent) : QAbstractListModel(parent)
{
}

int PublicGroupChatModel::rowCount(const QModelIndex &parent) const
{
	return parent == QModelIndex() ? m_groupChats.count() : 0;
}

QVariant PublicGroupChatModel::data(const QModelIndex &index, int role) const
{
	if (hasIndex(index.row(), index.column(), index.parent())) {
		const PublicGroupChat &groupChat = m_groupChats.at(index.row());

		switch (static_cast<CustomRole>(role)) {
		case CustomRole::Name:
			return groupChat.name();
		case CustomRole::Description:
			return groupChat.description();
		case CustomRole::Address:
			return groupChat.address();
		case CustomRole::Users:
			return groupChat.users();
		case CustomRole::IsOpen:
			return groupChat.isOpen();
		case CustomRole::Languages:
			return groupChat.languages();
		case CustomRole::GlobalSearch:
			break;
		case CustomRole::GroupChat:
			return QVariant::fromValue(groupChat);
		}
	}

	return {};
}

QHash<int, QByteArray> PublicGroupChatModel::roleNames() const
{
	static const QHash<int, QByteArray> roles = {
		{static_cast<int>(CustomRole::Name), QByteArrayLiteral("name")},
		{static_cast<int>(CustomRole::Description), QByteArrayLiteral("description")},
		{static_cast<int>(CustomRole::Address), QByteArrayLiteral("address")},
		{static_cast<int>(CustomRole::Users), QByteArrayLiteral("users")},
		{static_cast<int>(CustomRole::IsOpen), QByteArrayLiteral("isOpen")},
		{static_cast<int>(CustomRole::Languages), QByteArrayLiteral("languages")},
		{static_cast<int>(CustomRole::GlobalSearch), QByteArrayLiteral("globalSearch")},
		{static_cast<int>(CustomRole::GroupChat), QByteArrayLiteral("groupChat")},
	};

	return roles;
}

const PublicGroupChats &PublicGroupChatModel::groupChats() const
{
	return m_groupChats;
}

void PublicGroupChatModel::setGroupChats(const PublicGroupChats &groupChats)
{
	if (m_groupChats != groupChats) {
		beginResetModel();
		m_groupChats = groupChats;
		m_languages.clear();
		m_users = { 0, 0 };

		for (const PublicGroupChat &groupChat : groupChats) {
			const QStringList &groupChatLanguages = groupChat.languages();

			for (const QString &language : groupChatLanguages) {
				if (!language.isEmpty() && !m_languages.contains(language)) {
					m_languages.append(language);
				}
			}

			m_users.min = qMin(m_users.min, groupChat.users());
			m_users.max = qMax(m_users.max, groupChat.users());
		}

		m_languages.sort(Qt::CaseInsensitive);
		// Empty entry for *all*
		m_languages.prepend(QString());

		endResetModel();

		Q_EMIT groupChatsChanged(m_groupChats);
	}
}

int PublicGroupChatModel::count() const
{
	return m_groupChats.count();
}

QStringList PublicGroupChatModel::languages() const
{
	return m_languages;
}

int PublicGroupChatModel::minUsers() const
{
	return m_users.min;
}

int PublicGroupChatModel::maxUsers() const
{
	return m_users.max;
}

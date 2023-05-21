// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>

#include "PublicGroupChat.h"

class PublicGroupChatModel : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(PublicGroupChats groupChats READ groupChats WRITE setGroupChats NOTIFY groupChatsChanged)
	Q_PROPERTY(int count READ count NOTIFY groupChatsChanged)
	Q_PROPERTY(QStringList languages READ languages NOTIFY groupChatsChanged)
	Q_PROPERTY(int minUsers READ minUsers NOTIFY groupChatsChanged)
	Q_PROPERTY(int maxUsers READ maxUsers NOTIFY groupChatsChanged)

public:
	enum class CustomRole {
		Name = Qt::DisplayRole,
		Description = Qt::ToolTipRole,
		Address = Qt::UserRole,
		Users,
		IsOpen,
		Languages,
		GlobalSearch,
		GroupChat,
	};
	Q_ENUM(CustomRole)

	explicit PublicGroupChatModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = {}) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	const PublicGroupChats &groupChats() const;
	void setGroupChats(const PublicGroupChats &groupChats);
	Q_SIGNAL void groupChatsChanged(const PublicGroupChats &groupChats);

	int count() const;
	QStringList languages() const;
	int minUsers() const;
	int maxUsers() const;

private:
	PublicGroupChats m_groupChats;
	QStringList m_languages;
	struct {
		int min;
		int max;
	} m_users;
};

Q_DECLARE_METATYPE(PublicGroupChatModel::CustomRole)

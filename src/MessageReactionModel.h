// SPDX-FileCopyrightText: 2014 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include "MessageModel.h"

class MessageReactionModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(QString accountJid MEMBER accountJid)
	Q_PROPERTY(QString chatJid MEMBER chatJid)
	Q_PROPERTY(QVector<DetailedMessageReaction> reactions MEMBER reactions WRITE setReactions)

public:
	enum class Role {
		SenderJid = Qt::UserRole + 1,
		SenderName,
		Emojis,
	};

	MessageReactionModel(QObject *parent = nullptr);
	~MessageReactionModel() = default;

	[[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;
	[[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

	void setReactions(const QVector<DetailedMessageReaction> &reactions);

private:
	QString accountJid;
	QString chatJid;
	QVector<DetailedMessageReaction> reactions;
};

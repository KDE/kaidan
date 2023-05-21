// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>

class QXmppVCardIq;

class VCardModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)

public:
	enum Roles {
		Key,
		Value
	};

	class Item
	{
	public:
		Item() = default;
		Item(const QString &key, const QString &value);

		QString key() const;
		void setKey(const QString &key);

		QString value() const;
		void setValue(const QString &value);

	private:
		QString m_key;
		QString m_value;

	};

	explicit VCardModel(QObject *parent = nullptr);

	QHash<int, QByteArray> roleNames() const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	QString jid() const;
	void setJid(const QString &jid);

signals:
	void jidChanged();

private:
	void handleVCardReceived(const QXmppVCardIq &vCard);

	QVector<Item> m_vCard;
	QString m_jid;
};

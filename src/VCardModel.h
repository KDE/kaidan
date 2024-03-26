// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>

#include <QXmppVCardIq.h>

class QXmppVCardIq;

class VCardModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
	Q_PROPERTY(bool unsetEntriesProcessed MEMBER m_unsetEntriesProcessed NOTIFY unsetEntriesProcessedChanged)

public:
	enum Roles {
		Key,
		Value,
		UriScheme,
	};

	struct Item {
		QString name;
		std::function<QString(const QXmppVCardIq *)> value;
		std::function<void(QXmppVCardIq *, const QString &)> setValue;
		QString uriScheme = {};
	};

	explicit VCardModel(QObject *parent = nullptr);

	QHash<int, QByteArray> roleNames() const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	QString jid() const;
	void setJid(const QString &jid);

	void generateEntries();

Q_SIGNALS:
	void jidChanged();
	void unsetEntriesProcessedChanged();

private:
	void handleVCardReceived(const QXmppVCardIq &vCard);

	QString m_jid;
	bool m_unsetEntriesProcessed = false;
	QXmppVCardIq m_vCard;
	QVector<Item> m_vCardMap;
};

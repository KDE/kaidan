// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "VCardModel.h"

#include <QXmppVCardIq.h>

#include "Kaidan.h"
#include "VCardManager.h"

VCardModel::VCardModel(QObject *parent)
	: QAbstractListModel(parent)
{
	connect(
		Kaidan::instance()->client()->vCardManager(),
		&VCardManager::vCardReceived,
		this,
		&VCardModel::handleVCardReceived
	);
}

QHash<int, QByteArray> VCardModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[Key] = "key";
	roles[Value] = "value";
	return roles;
}

int VCardModel::rowCount(const QModelIndex &parent) const
{
	// For list models only the root node (an invalid parent) should return the
	// list's size. For all other (valid) parents, rowCount() should return 0 so
	// that it does not become a tree model.
	if (parent.isValid())
		return 0;

	return m_vCard.size();
}

QVariant VCardModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return {};

	switch(role) {
	case Key:
		return m_vCard.at(index.row()).key();
	case Value:
		return m_vCard.at(index.row()).value();
	}
	return {};
}

void VCardModel::handleVCardReceived(const QXmppVCardIq &vCard)
{
	if (vCard.from() == m_jid) {
		beginResetModel();

		m_vCard.clear();

		if (!vCard.fullName().isEmpty())
			m_vCard << Item(tr("Name"), vCard.fullName());

		if (!vCard.nickName().isEmpty())
			m_vCard << Item(tr("Nickname"), vCard.nickName());

		if (!vCard.description().isEmpty())
			m_vCard << Item(tr("About"), vCard.description());

		if (!vCard.email().isEmpty())
			m_vCard << Item(tr("Email"), vCard.email());

		if (!vCard.birthday().isNull() && vCard.birthday().isValid())
			m_vCard << Item(tr("Birthday"), vCard.birthday().toString());

		if (!vCard.url().isEmpty())
			m_vCard << Item(tr("Website"), vCard.url());

		endResetModel();
	}
}

QString VCardModel::jid() const
{
	return m_jid;
}

void VCardModel::setJid(const QString &jid)
{
	m_jid = jid;
	emit jidChanged();

	if (Kaidan::instance()->connectionState() == QXmppClient::ConnectedState) {
		emit Kaidan::instance()->client()->vCardManager()->vCardRequested(jid);
	}
}

VCardModel::Item::Item(const QString &key, const QString &value)
	: m_key(key), m_value(value)
{
}

QString VCardModel::Item::key() const
{
	return m_key;
}

void VCardModel::Item::setKey(const QString &key)
{
	m_key = key;
}

QString VCardModel::Item::value() const
{
	return m_value;
}

void VCardModel::Item::setValue(const QString &value)
{
	m_value = value;
}

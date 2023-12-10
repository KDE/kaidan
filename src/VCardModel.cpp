// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "VCardModel.h"

#include <QXmppVCardIq.h>
#include <QXmppVCardManager.h>

#include "Kaidan.h"
#include "VCardManager.h"
#include "FutureUtils.h"

using namespace Enums;

VCardModel::VCardModel(QObject *parent)
	: QAbstractListModel(parent)
{
	connect(
		Kaidan::instance()->client()->vCardManager(),
		&VCardManager::vCardReceived,
		this,
		&VCardModel::handleVCardReceived
	);
	connect(this, &VCardModel::unsetEntriesProcessedChanged, this, [this]() {
		beginResetModel();
		generateEntries();
		endResetModel();
	});
}

QHash<int, QByteArray> VCardModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[Key] = "key";
	roles[Value] = "value";
	roles[UriScheme] = "uriScheme";
	return roles;
}

int VCardModel::rowCount(const QModelIndex &parent) const
{
	// For list models only the root node (an invalid parent) should return the
	// list's size. For all other (valid) parents, rowCount() should return 0 so
	// that it does not become a tree model.
	if (parent.isValid())
		return 0;

	return m_vCardMap.size();
}

QVariant VCardModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return {};

	const auto item = m_vCardMap.at(index.row());

	switch(role) {
	case Key:
		return item.name;
	case Value:
		return item.value(&m_vCard);
	case UriScheme:
		return item.uriScheme;
	}
	return {};
}

bool VCardModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	int i = index.row();

	if (role == Value) {
		m_vCardMap.at(i).setValue(&m_vCard, value.toString());
		Q_EMIT dataChanged(this->index(i), this->index(i), {Roles::Value});

		auto *clientWorker = Kaidan::instance()->client();
		runOnThread(clientWorker, [clientWorker, vCard = m_vCard]() {
			clientWorker->startTask([=]() {
				Kaidan::instance()
				        ->client()
				        ->xmppClient()
				        ->findExtension<QXmppVCardManager>()
				        ->setClientVCard(vCard);
				clientWorker->finishTask();
			});
		});
		return true;
	}

	return false;
}

void VCardModel::handleVCardReceived(const QXmppVCardIq &vCard)
{
	if (vCard.from() == m_jid) {
		beginResetModel();
		m_vCard = vCard;
		generateEntries();
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
	Q_EMIT jidChanged();

	if (Kaidan::instance()->connectionState() == ConnectionState::StateConnected) {
		Q_EMIT Kaidan::instance()->client()->vCardManager()->vCardRequested(jid);
	}
}

void VCardModel::generateEntries()
{
	m_vCardMap.clear();

	auto fullName = Item { tr("Name"), &QXmppVCardIq::fullName,  &QXmppVCardIq::setFullName };
	auto nickName = Item { tr("Nickname"), &QXmppVCardIq::nickName,  &QXmppVCardIq::setNickName };
	auto description = Item { tr("About"), &QXmppVCardIq::description,  &QXmppVCardIq::setDescription };
	auto email = Item { tr("Email"), &QXmppVCardIq::email, &QXmppVCardIq::setEmail, QStringLiteral("mailto") };
	auto birthday = Item { tr("Birthday"),
		[](const QXmppVCardIq *vCard) { return vCard->birthday().toString(); },
		[](QXmppVCardIq *vCard, const QString &d) { return vCard->setBirthday(QDate::fromString(d, Qt::ISODate)); }
	};
	auto url = Item { tr("Website"), &QXmppVCardIq::url, &QXmppVCardIq::setUrl, QStringLiteral("http") };

	if (m_unsetEntriesProcessed) {
		m_vCardMap = { fullName, nickName, description, email, birthday, url };
	} else {
	    if (!m_vCard.fullName().isEmpty())
	        m_vCardMap << fullName;
	    if (!m_vCard.nickName().isEmpty())
	        m_vCardMap << nickName;
	    if (!m_vCard.description().isEmpty())
	        m_vCardMap << description;
	    if (!m_vCard.email().isEmpty())
	        m_vCardMap << email;
	    if (!m_vCard.birthday().isNull() && m_vCard.birthday().isValid())
	        m_vCardMap << birthday;
	    if (!m_vCard.url().isEmpty())
	        m_vCardMap << url;
	}
}


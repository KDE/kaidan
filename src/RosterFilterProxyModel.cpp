// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterFilterProxyModel.h"

#include "PresenceCache.h"
#include "RosterModel.h"

RosterFilterProxyModel::RosterFilterProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent)
{
}

void RosterFilterProxyModel::setOnlyAvailableContactsShown(bool onlyAvailableContactsShown)
{
	if (m_onlyAvailableContactsShown != onlyAvailableContactsShown) {
		m_onlyAvailableContactsShown = onlyAvailableContactsShown;
		invalidate();
		Q_EMIT onlyAvailableContactsShownChanged();
	}
}

bool RosterFilterProxyModel::onlyAvailableContactsShown() const
{
	return m_onlyAvailableContactsShown;
}

void RosterFilterProxyModel::setSelectedAccountJids(const QVector<QString> &selectedAccountJids)
{
	if (m_selectedAccountJids != selectedAccountJids) {
		m_selectedAccountJids = selectedAccountJids;
		invalidate();
		Q_EMIT selectedAccountJidsChanged();
	}

}

QVector<QString> RosterFilterProxyModel::selectedAccountJids() const
{
	return m_selectedAccountJids;
}

void RosterFilterProxyModel::setSelectedGroups(const QVector<QString> &selectedGroups)
{
	if (m_selectedGroups != selectedGroups) {
		m_selectedGroups = selectedGroups;
		invalidate();
		Q_EMIT selectedGroupsChanged();
	}

}

QVector<QString> RosterFilterProxyModel::selectedGroups() const
{
	return m_selectedGroups;
}

bool RosterFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

	if (m_onlyAvailableContactsShown) {
		auto *presenceCache = PresenceCache::instance();
		const auto chatJid = sourceModel()->data(index, RosterModel::JidRole).toString();

		if (const auto contactPresence = presenceCache->presence(chatJid, presenceCache->pickIdealResource(chatJid))) {
			if (contactPresence->type() != QXmppPresence::Available) {
				return false;
			}
		} else {
			return false;
		}
	}

	if (const auto accountJid = sourceModel()->data(index, RosterModel::AccountJidRole).toString();
		!m_selectedAccountJids.isEmpty() && !m_selectedAccountJids.contains(accountJid)) {
		return false;
	}

	if (const auto groups = sourceModel()->data(index, RosterModel::GroupsRole).value<QVector<QString>>();
		!m_selectedGroups.isEmpty() && std::none_of(groups.cbegin(), groups.cend(), [&](const QString &group) {
		return m_selectedGroups.contains(group);
	})) {
		return false;
	}

	return sourceModel()->data(index, RosterModel::NameRole).toString().toLower().contains(filterRegExp()) ||
		   sourceModel()->data(index, RosterModel::JidRole).toString().toLower().contains(filterRegExp());
}

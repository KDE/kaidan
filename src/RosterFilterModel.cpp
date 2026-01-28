// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterFilterModel.h"

// Kaidan
#include "Account.h"
#include "AccountController.h"
#include "Algorithms.h"
#include "PresenceCache.h"
#include "RosterModel.h"

RosterFilterModel::RosterFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    connect(this, &QSortFilterProxyModel::sourceModelChanged, this, [this]() {
        connect(AccountController::instance(), &AccountController::accountsChanged, this, &RosterFilterModel::updateSelectedAccountJids);
        connect(static_cast<RosterModel *>(sourceModel()), &RosterModel::groupsChanged, this, &RosterFilterModel::updateSelectedGroups);
    });
}

void RosterFilterModel::addDisplayedType(Type type)
{
    if (!m_displayedTypes.testFlag(type)) {
        m_displayedTypes.setFlag(type);
        invalidate();
        Q_EMIT displayedTypesChanged();
    }
}

void RosterFilterModel::removeDisplayedType(Type type)
{
    if (m_displayedTypes.testFlag(type)) {
        m_displayedTypes.setFlag(type, false);
        invalidate();
        Q_EMIT displayedTypesChanged();
    }
}

void RosterFilterModel::resetDisplayedTypes()
{
    if (m_displayedTypes) {
        m_displayedTypes = {};
        invalidate();
        Q_EMIT displayedTypesChanged();
    }
}

RosterFilterModel::Types RosterFilterModel::displayedTypes() const
{
    return m_displayedTypes;
}

void RosterFilterModel::setSelectedAccountJids(const QList<QString> &selectedAccountJids)
{
    if (m_selectedAccountJids != selectedAccountJids) {
        m_selectedAccountJids = selectedAccountJids;
        invalidate();
        Q_EMIT selectedAccountJidsChanged();
    }
}

QList<QString> RosterFilterModel::selectedAccountJids() const
{
    return m_selectedAccountJids;
}

void RosterFilterModel::setSelectedGroups(const QList<QString> &selectedGroups)
{
    if (m_selectedGroups != selectedGroups) {
        m_selectedGroups = selectedGroups;
        invalidate();
        Q_EMIT selectedGroupsChanged();
    }
}

QList<QString> RosterFilterModel::selectedGroups() const
{
    return m_selectedGroups;
}

void RosterFilterModel::reorderPinnedItem(int oldIndex, int newIndex)
{
    const int sourceOldIndex = mapToSource(index(oldIndex, 0)).row();
    const int sourceNewIndex = mapToSource(index(newIndex, 0)).row();
    static_cast<RosterModel *>(sourceModel())->reorderPinnedItem(sourceOldIndex, sourceNewIndex);
}

bool RosterFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    if (m_displayedTypes) {
        if (sourceModel()->data(index, RosterModel::IsGroupChatRole).toBool()) {
            if (m_displayedTypes.testAnyFlags(Type::PrivateGroupChat | Type::PublicGroupChat)) {
                if (sourceModel()->data(index, RosterModel::IsPublicGroupChatRole).toBool()) {
                    if (!m_displayedTypes.testFlag(Type::PublicGroupChat)) {
                        return false;
                    }
                } else if (!m_displayedTypes.testFlag(Type::PrivateGroupChat)) {
                    return false;
                }
            } else {
                return false;
            }
        } else if (m_displayedTypes.testAnyFlags(Type::AvailableContact | Type::UnavailableContact)) {
            const auto *account = sourceModel()->data(index, RosterModel::AccountRole).value<Account *>();
            const auto chatJid = sourceModel()->data(index, RosterModel::JidRole).toString();

            if (account->settings()->jid() == chatJid) {
                return false;
            }

            const auto presenceCache = account->presenceCache();

            if (const auto contactPresence = presenceCache->presence(chatJid)) {
                if (contactPresence->type() == QXmppPresence::Available) {
                    if (!m_displayedTypes.testFlag(Type::AvailableContact)) {
                        return false;
                    }
                } else if (!m_displayedTypes.testFlag(Type::UnavailableContact)) {
                    return false;
                }
            } else if (!m_displayedTypes.testFlag(Type::UnavailableContact)) {
                return false;
            }
        } else {
            return false;
        }
    }

    if (const auto accountJid = sourceModel()->data(index, RosterModel::AccountRole).value<Account *>()->settings()->jid();
        !m_selectedAccountJids.isEmpty() && !m_selectedAccountJids.contains(accountJid)) {
        return false;
    }

    if (const auto groups = sourceModel()->data(index, RosterModel::GroupsRole).value<QList<QString>>();
        !m_selectedGroups.isEmpty() && std::ranges::none_of(groups, [&](const QString &group) {
            return m_selectedGroups.contains(group);
        })) {
        return false;
    }

    return sourceModel()->data(index, RosterModel::NameRole).toString().toLower().contains(filterRegularExpression())
        || sourceModel()->data(index, RosterModel::JidRole).toString().toLower().contains(filterRegularExpression());
}

void RosterFilterModel::updateSelectedAccountJids()
{
    const auto accountJids = transform(AccountController::instance()->accounts(), [](Account *account) {
        return account->settings()->jid();
    });

    bool changed = false;

    // Remove selected account JIDs that have been removed from Kaidan.
    for (auto itr = m_selectedAccountJids.begin(); itr != m_selectedAccountJids.end();) {
        if (accountJids.contains(*itr)) {
            ++itr;
        } else {
            itr = m_selectedAccountJids.erase(itr);
            changed = true;
        }
    }

    if (changed) {
        invalidate();
        Q_EMIT selectedAccountJidsChanged();
    }
}

void RosterFilterModel::updateSelectedGroups()
{
    const auto groups = static_cast<RosterModel *>(sourceModel())->groups();
    bool changed = false;

    // Remove selected groups that have been removed from the source model.
    for (auto itr = m_selectedGroups.begin(); itr != m_selectedGroups.end();) {
        if (groups.contains(*itr)) {
            ++itr;
        } else {
            itr = m_selectedGroups.erase(itr);
            changed = true;
        }
    }

    if (changed) {
        invalidate();
        Q_EMIT selectedGroupsChanged();
    }
}

#include "moc_RosterFilterModel.cpp"

// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterFilterProxyModel.h"

#include "GroupChatController.h"
#include "PresenceCache.h"
#include "RosterModel.h"

RosterFilterProxyModel::RosterFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    connect(GroupChatController::instance(), &GroupChatController::currentUserJidsChanged, this, &RosterFilterProxyModel::updateGroupChatUserJids);
    updateGroupChatUserJids();
}

void RosterFilterProxyModel::addDisplayedType(Type type)
{
    if (!m_displayedTypes.testFlag(type)) {
        m_displayedTypes.setFlag(type);
        invalidateFilter();
        Q_EMIT displayedTypesChanged();
    }
}

void RosterFilterProxyModel::removeDisplayedType(Type type)
{
    if (m_displayedTypes.testFlag(type)) {
        m_displayedTypes.setFlag(type, false);
        invalidateFilter();
        Q_EMIT displayedTypesChanged();
    }
}

void RosterFilterProxyModel::resetDisplayedTypes()
{
    if (m_displayedTypes) {
        m_displayedTypes = {};
        invalidateFilter();
        Q_EMIT displayedTypesChanged();
    }
}

RosterFilterProxyModel::Types RosterFilterProxyModel::displayedTypes() const
{
    return m_displayedTypes;
}

void RosterFilterProxyModel::setSelectedAccountJids(const QList<QString> &selectedAccountJids)
{
    if (m_selectedAccountJids != selectedAccountJids) {
        m_selectedAccountJids = selectedAccountJids;
        invalidateFilter();
        Q_EMIT selectedAccountJidsChanged();
    }
}

QList<QString> RosterFilterProxyModel::selectedAccountJids() const
{
    return m_selectedAccountJids;
}

void RosterFilterProxyModel::setSelectedGroups(const QList<QString> &selectedGroups)
{
    if (m_selectedGroups != selectedGroups) {
        m_selectedGroups = selectedGroups;
        invalidateFilter();
        Q_EMIT selectedGroupsChanged();
    }
}

QList<QString> RosterFilterProxyModel::selectedGroups() const
{
    return m_selectedGroups;
}

void RosterFilterProxyModel::setGroupChatsExcluded(bool groupChatsExcluded)
{
    if (m_groupChatsExcluded != groupChatsExcluded) {
        m_groupChatsExcluded = groupChatsExcluded;
        invalidateFilter();
        Q_EMIT groupChatsExcludedChanged();
    }
}

bool RosterFilterProxyModel::groupChatsExcluded()
{
    return m_groupChatsExcluded;
}

void RosterFilterProxyModel::setGroupChatUsersExcluded(bool groupChatUsersExcluded)
{
    if (m_groupChatUsersExcluded != groupChatUsersExcluded) {
        m_groupChatUsersExcluded = groupChatUsersExcluded;
        invalidateFilter();
        Q_EMIT groupChatUsersExcludedChanged();
    }
}

bool RosterFilterProxyModel::groupChatUsersExcluded()
{
    return m_groupChatUsersExcluded;
}

void RosterFilterProxyModel::reorderPinnedItem(const QString &accountJid, const QString &jid, int oldIndex, int newIndex)
{
    auto rosterModel = static_cast<RosterModel *>(sourceModel());
    const int sourceOldIndex = mapToSource(index(oldIndex, 0)).row();
    const int sourceNewIndex = mapToSource(index(newIndex, 0)).row();
    rosterModel->reorderPinnedItem(accountJid, jid, sourceOldIndex, sourceNewIndex);
}

bool RosterFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    if (m_displayedTypes) {
        auto typeAccepted = false;
        const auto isGroupChat = sourceModel()->data(index, RosterModel::IsGroupChatRole).toBool();

        if ((m_displayedTypes.testFlag(Type::PrivateGroupChat) || m_displayedTypes.testFlag(Type::PublicGroupChat)) && isGroupChat) {
            const auto isPublicGroupChat = sourceModel()->data(index, RosterModel::IsPublicGroupChatRole).toBool();
            typeAccepted = (m_displayedTypes.testFlag(Type::PrivateGroupChat) && !isPublicGroupChat)
                || (m_displayedTypes.testFlag(Type::PublicGroupChat) && isPublicGroupChat);
        }

        if (!typeAccepted && (m_displayedTypes.testFlag(Type::UnavailableContact) || m_displayedTypes.testFlag(Type::AvailableContact)) && !isGroupChat) {
            auto *presenceCache = PresenceCache::instance();
            const auto chatJid = sourceModel()->data(index, RosterModel::JidRole).toString();

            if (const auto contactPresence = presenceCache->presence(chatJid, presenceCache->pickIdealResource(chatJid))) {
                typeAccepted = (m_displayedTypes.testFlag(Type::UnavailableContact) && contactPresence->type() == QXmppPresence::Unavailable)
                    || (m_displayedTypes.testFlag(Type::AvailableContact) && contactPresence->type() == QXmppPresence::Available);
            } else {
                typeAccepted = m_displayedTypes.testFlag(Type::UnavailableContact);
            }
        }

        if (!typeAccepted) {
            return false;
        }
    }

    if (const auto accountJid = sourceModel()->data(index, RosterModel::AccountJidRole).toString();
        !m_selectedAccountJids.isEmpty() && !m_selectedAccountJids.contains(accountJid)) {
        return false;
    }

    if (const auto groups = sourceModel()->data(index, RosterModel::GroupsRole).value<QList<QString>>();
        !m_selectedGroups.isEmpty() && std::none_of(groups.cbegin(), groups.cend(), [&](const QString &group) {
            return m_selectedGroups.contains(group);
        })) {
        return false;
    }

    if (m_groupChatsExcluded && sourceModel()->data(index, RosterModel::IsGroupChatRole).toBool()) {
        return false;
    }

    // TODO: Create GroupChatInviteeFilterModel and move all related things into that class
    if (m_groupChatUsersExcluded && m_groupChatUserJids.contains(sourceModel()->data(index, RosterModel::JidRole).toString())) {
        return false;
    }

    return sourceModel()->data(index, RosterModel::NameRole).toString().toLower().contains(filterRegularExpression())
        || sourceModel()->data(index, RosterModel::JidRole).toString().toLower().contains(filterRegularExpression());
}

void RosterFilterProxyModel::updateGroupChatUserJids()
{
    m_groupChatUserJids = GroupChatController::instance()->currentUserJids();
    invalidateFilter();
}

#include "moc_RosterFilterProxyModel.cpp"

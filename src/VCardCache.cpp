// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "VCardCache.h"

// Qt
#include <QMutexLocker>

VCardCache::VCardCache(QObject *parent)
	: QObject(parent)
{
}

std::optional<QXmppVCardIq> VCardCache::vCard(const QString &jid) const
{
	QMutexLocker locker(&m_mutex);
	if (m_vCards.contains(jid))
		return m_vCards.value(jid);
	return std::nullopt;
}

void VCardCache::setVCard(const QString &jid, const QXmppVCardIq &vCard)
{
	QMutexLocker locker(&m_mutex);
	if (m_vCards.value(jid) != vCard) {
		m_vCards.insert(jid, vCard);
		locker.unlock();
		emit vCardChanged(jid);
	}
}

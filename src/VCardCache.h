// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <optional>
// Qt
#include <QHash>
#include <QMutex>
#include <QObject>
// QXmpp
#include <QXmppVCardIq.h>

class VCardCache : public QObject
{
	Q_OBJECT

public:
	VCardCache(QObject* parent = nullptr);

	/**
	 * Returns the vCard for a JID.
	 *
	 * This method is thread-safe.
	 *
	 * @param jid JID for which the vCard is retrieved
	 */
	std::optional<QXmppVCardIq> vCard(const QString &jid) const;

	/**
	 * Sets the vCard for a JID.
	 *
	 * This method is thread-safe.
	 *
	 * @param jid JID to which the vCard belongs
	 * @param vCard vCard being set
	 */
	void setVCard(const QString &jid, const QXmppVCardIq &vCard);

signals:
	/**
	 * Emitted when a vCard changed.
	 *
	 * @param jid JID of the changed vCard
	 */
	void vCardChanged(const QString &jid);

private:
	mutable QMutex m_mutex;

	// mapping from a JID to the vCard of that JID
	QHash<QString, QXmppVCardIq> m_vCards;
};

// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TrustMessageUriGenerator.h"

#include "Globals.h"
#include "qxmpp-exts/QXmppUri.h"

TrustMessageUriGenerator::TrustMessageUriGenerator(QObject *parent)
	: QObject(parent)
{
}

void TrustMessageUriGenerator::setJid(const QString &jid)
{
	if (m_jid != jid) {
		m_jid = jid;
		Q_EMIT jidChanged();
	}
}

QString TrustMessageUriGenerator::uri() const
{
	QXmppUri uri;
	uri.setJid(m_jid);

	// Create a Trust Message URI only if there are keys for it.
	if (!m_authenticatedKeys.isEmpty() || !m_distrustedKeys.isEmpty()) {
		uri.setAction(QXmppUri::TrustMessage);
		uri.setEncryption(XMLNS_OMEMO_2);
		uri.setTrustedKeysIds(m_authenticatedKeys);
		uri.setDistrustedKeysIds(m_distrustedKeys);
	}

	return uri.toString();
}

QString TrustMessageUriGenerator::jid() const
{
	return m_jid;
}

void TrustMessageUriGenerator::setKeys(const QList<QString> &authenticatedKeys, const QList<QString> &distrustedKeys)
{
	bool changed = false;

	if (m_authenticatedKeys != authenticatedKeys) {
		m_authenticatedKeys = authenticatedKeys;
		changed = true;
	}

	if (m_distrustedKeys != distrustedKeys) {
		m_distrustedKeys = distrustedKeys;
		changed = true;
	}

	// If a contact has no keys, uriChanged() must be emitted as well for the initial update.
	if (changed || (m_authenticatedKeys.isEmpty() && m_distrustedKeys.isEmpty())) {
		Q_EMIT uriChanged();
	}
}

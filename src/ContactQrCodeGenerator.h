// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "AbstractQrCodeGenerator.h"
#include "ContactTrustMessageUriGenerator.h"

/**
 * Gerenates a QR code encoding the Trust Message URI of a contact.
 *
 * If no keys for the Trust Message URI can be found, an XMPP URI containing only the contact's bare
 * JID is used.
 */
class ContactQrCodeGenerator : public AbstractQrCodeGenerator
{
	Q_OBJECT

	Q_PROPERTY(QString accountJid READ accountJid WRITE setAccountJid NOTIFY accountJidChanged)

public:
	explicit ContactQrCodeGenerator(QObject *parent = nullptr);

	QString accountJid() const;
	void setAccountJid(const QString &accountJid);
	Q_SIGNAL void accountJidChanged();

private:
	void updateUriAccountJid();
	void updateUriJid();
	void updateText();

	QString m_accountJid;
	ContactTrustMessageUriGenerator m_uriGenerator;
};

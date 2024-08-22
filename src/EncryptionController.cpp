// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EncryptionController.h"

#include "OmemoController.h"

EncryptionController *EncryptionController::s_instance = nullptr;

EncryptionController *EncryptionController::instance()
{
	return s_instance;
}

EncryptionController::EncryptionController(QObject *parent)
	: QObject(parent), m_omemoController(std::make_unique<OmemoController>())
{
	Q_ASSERT(!EncryptionController::s_instance);
	s_instance = this;
}

EncryptionController::~EncryptionController()
{
	s_instance = nullptr;
}

QFuture<void> EncryptionController::load()
{
	return m_omemoController->load();
}

QFuture<void> EncryptionController::setUp()
{
	return m_omemoController->setUp();
}

void EncryptionController::initializeAccountFromQml(const QString &accountJid)
{
	initializeAccount(accountJid);
}

QFuture<void> EncryptionController::initializeAccount(const QString &accountJid)
{
	return m_omemoController->initializeAccount(accountJid);
}

QFuture<void> EncryptionController::initializeChat(const QString &accountJid, const QList<QString> &contactJids)
{
	return m_omemoController->initializeChat(accountJid, contactJids);
}

QFuture<QString> EncryptionController::ownKey(const QString &accountJid)
{
	return m_omemoController->ownKey(accountJid);
}

QFuture<QHash<QString, QHash<QString, QXmpp::TrustLevel>>> EncryptionController::keys(const QString &accountJid, const QList<QString> &jids, QXmpp::TrustLevels trustLevels)
{
	return m_omemoController->keys(accountJid, jids, trustLevels);
}

QFuture<EncryptionController::OwnDevice> EncryptionController::ownDevice(const QString &accountJid)
{
	return m_omemoController->ownDevice(accountJid);
}

QFuture<QList<EncryptionController::Device>> EncryptionController::devices(const QString &accountJid, const QList<QString> &jids)
{
	return m_omemoController->devices(accountJid, jids);
}

QFuture<bool> EncryptionController::hasUsableDevices(const QList<QString> &jids)
{
	return m_omemoController->hasUsableDevices(jids);
}

void EncryptionController::removeContactDevices(const QString &jid)
{
	m_omemoController->removeContactDevices(jid);
}

QFuture<void> EncryptionController::unload()
{
	return m_omemoController->unsubscribeFromDeviceLists();
}

QFuture<void> EncryptionController::reset()
{
	return m_omemoController->reset();
}

QFuture<void> EncryptionController::resetLocally()
{
	return m_omemoController->resetLocally();
}

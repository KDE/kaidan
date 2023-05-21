// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RegistrationManager.h"

// QXmpp
#include <QXmppBitsOfBinaryDataList.h>
#include <QXmppRegistrationManager.h>
// Kaidan
#include "AccountManager.h"
#include "BitsOfBinaryImageProvider.h"
#include "ClientWorker.h"
#include "Kaidan.h"
#include "RegistrationDataFormModel.h"
#include "ServerFeaturesCache.h"

RegistrationManager::RegistrationManager(ClientWorker *clientWorker, QXmppClient *client, QObject *parent)
	: QObject(parent),
	  m_clientWorker(clientWorker),
	  m_client(client),
	  m_manager(new QXmppRegistrationManager),
	  m_dataFormModel(new RegistrationDataFormModel())
{
	client->addExtension(m_manager);

	connect(m_manager, &QXmppRegistrationManager::supportedByServerChanged, this, &RegistrationManager::handleInBandRegistrationSupportedChanged);

	// account creation
	connect(this, &RegistrationManager::sendRegistrationFormRequested,
		this, &RegistrationManager::sendRegistrationForm);
	connect(m_manager, &QXmppRegistrationManager::registrationFormReceived, this, &RegistrationManager::handleRegistrationFormReceived);
	connect(m_manager, &QXmppRegistrationManager::registrationSucceeded, this, &RegistrationManager::handleRegistrationSucceeded);
	connect(m_manager, &QXmppRegistrationManager::registrationFailed, this, &RegistrationManager::handleRegistrationFailed);

	// account deletion
	connect(m_manager, &QXmppRegistrationManager::accountDeletionFailed, m_clientWorker, &ClientWorker::handleAccountDeletionFromServerFailed);
	connect(m_manager, &QXmppRegistrationManager::accountDeleted, m_clientWorker, &ClientWorker::handleAccountDeletedFromServer);

	// password change
	connect(this, &RegistrationManager::changePasswordRequested,
		this, &RegistrationManager::changePassword);
	connect(m_manager, &QXmppRegistrationManager::passwordChanged, this, &RegistrationManager::handlePasswordChanged);
	connect(m_manager, &QXmppRegistrationManager::passwordChangeFailed, this, &RegistrationManager::handlePasswordChangeFailed);
}

void RegistrationManager::setRegisterOnConnectEnabled(bool registerOnConnect)
{
	m_manager->setRegisterOnConnectEnabled(registerOnConnect);
}

void RegistrationManager::deleteAccount()
{
	m_manager->deleteAccount();
}

void RegistrationManager::sendRegistrationForm()
{
	if (m_dataFormModel->isFakeForm()) {
		QXmppRegisterIq iq;
		iq.setUsername(m_dataFormModel->extractUsername());
		iq.setPassword(m_dataFormModel->extractPassword());
		iq.setEmail(m_dataFormModel->extractEmail());

		m_manager->setRegistrationFormToSend(iq);
	} else {
		m_manager->setRegistrationFormToSend(m_dataFormModel->form());
	}

	m_clientWorker->connectToRegister();
}

void RegistrationManager::changePassword(const QString &newPassword)
{
	m_clientWorker->startTask(
		[this, newPassword] {
			m_manager->changePassword(newPassword);
		}
	);
}

void RegistrationManager::handleInBandRegistrationSupportedChanged()
{
	if (m_client->isConnected()) {
		m_clientWorker->caches()->serverFeaturesCache->setInBandRegistrationSupported(m_manager->supportedByServer());
	}
}

void RegistrationManager::handleRegistrationFormReceived(const QXmppRegisterIq &iq)
{
	m_client->disconnectFromServer();

	bool isFakeForm;
	QXmppDataForm newDataForm = extractFormFromRegisterIq(iq, isFakeForm);

	// If there is no registration data form, try to use an out-of-band URL.
	if (newDataForm.fields().isEmpty()) {
		// If there is a standardized out-of-band URL, use that.
		if (!iq.outOfBandUrl().isEmpty()) {
			emit Kaidan::instance()->registrationOutOfBandUrlReceived(iq.outOfBandUrl());
			setRegisterOnConnectEnabled(false);
			return;
		}

		// Try to find an out-of-band URL within the instructions element.
		// Most servers include a text with a link to the website.
		const auto words = iq.instructions().split(u' ');
		for (const auto &instructionPart : words) {
			if (instructionPart.startsWith(u"https://")) {
				emit Kaidan::instance()->registrationOutOfBandUrlReceived(instructionPart);
				setRegisterOnConnectEnabled(false);
				return;
			}
		}

		// If no URL has been found in the instructions, there is a
		// problem with the server.
		emit m_clientWorker->connectionErrorChanged(ClientWorker::RegistrationUnsupported);
		setRegisterOnConnectEnabled(false);
		return;
	}

	copyUserDefinedValuesToNewForm(m_dataFormModel->form(), newDataForm);
	cleanUpLastForm();

	m_dataFormModel = new RegistrationDataFormModel(newDataForm);
	m_dataFormModel->setIsFakeForm(isFakeForm);
	// Move to the main thread, so QML can connect signals to the model.
	m_dataFormModel->moveToThread(Kaidan::instance()->thread());

	// Add the attached Bits of Binary data to the corresponding image provider.
	const auto bobDataList = iq.bitsOfBinaryData();
	for (const auto &bobData : bobDataList) {
		BitsOfBinaryImageProvider::instance()->addImage(bobData);
		m_contentIdsToRemove << bobData.cid();
	}

	emit Kaidan::instance()->registrationFormReceived(m_dataFormModel);
}

void RegistrationManager::handleRegistrationSucceeded()
{
	AccountManager::instance()->setJid(m_dataFormModel->extractUsername().append('@').append(m_client->configuration().domain()));
	AccountManager::instance()->setPassword(m_dataFormModel->extractPassword());

	m_client->disconnectFromServer();
	setRegisterOnConnectEnabled(false);
	m_clientWorker->logIn();

	cleanUpLastForm();
	m_dataFormModel = new RegistrationDataFormModel();
}

void RegistrationManager::handleRegistrationFailed(const QXmppStanza::Error &error)
{
	m_client->disconnectFromServer();
	setRegisterOnConnectEnabled(false);

	RegistrationError registrationError = RegistrationError::UnknownError;

	switch(error.type()) {
	case QXmppStanza::Error::Cancel:
		if (error.condition() == QXmppStanza::Error::FeatureNotImplemented)
			registrationError = RegistrationError::InBandRegistrationNotSupported;
		else if (error.condition() == QXmppStanza::Error::Conflict)
			registrationError = RegistrationError::UsernameConflict;
		else if (error.condition() == QXmppStanza::Error::NotAllowed && error.text().contains("captcha", Qt::CaseInsensitive))
			registrationError = RegistrationError::CaptchaVerificationFailed;
		break;
	case QXmppStanza::Error::Modify:
		if (error.condition() == QXmppStanza::Error::NotAcceptable) {
			// TODO: Check error text in English (needs QXmpp change)
			if (error.text().contains("password", Qt::CaseInsensitive) && (error.text().contains("weak", Qt::CaseInsensitive) || error.text().contains("short", Qt::CaseInsensitive)))
				registrationError = RegistrationError::PasswordTooWeak;
			else if (error.text().contains("ip", Qt::CaseInsensitive) || error.text().contains("quickly", Qt::CaseInsensitive)
)
				registrationError = RegistrationError::TemporarilyBlocked;
			else
				registrationError = RegistrationError::RequiredInformationMissing;
		} else if (error.condition() == QXmppStanza::Error::BadRequest && error.text().contains("captcha", Qt::CaseInsensitive)) {
			registrationError = RegistrationError::CaptchaVerificationFailed;
		}
		break;
	default:
		break;
	}

	emit Kaidan::instance()->registrationFailed(quint8(registrationError), error.text());
}

void RegistrationManager::handlePasswordChanged(const QString &newPassword)
{
	AccountManager::instance()->setPassword(newPassword);
	AccountManager::instance()->storePassword();
	AccountManager::instance()->setHasNewCredentials(false);
	m_clientWorker->finishTask();
	emit Kaidan::instance()->passwordChangeSucceeded();
}

void RegistrationManager::handlePasswordChangeFailed(const QXmppStanza::Error &error)
{
	emit Kaidan::instance()->passwordChangeFailed(error.text());
	m_clientWorker->finishTask();
}

QXmppDataForm RegistrationManager::extractFormFromRegisterIq(const QXmppRegisterIq& iq, bool &isFakeForm)
{
	QXmppDataForm newDataForm = iq.form();
	if (newDataForm.fields().isEmpty()) {
		// This is a hack, so we only need to implement one way of registering in QML.
		// A 'fake' data form model is created with a username and password field.
		isFakeForm = true;

		if (!iq.username().isNull()) {
			QXmppDataForm::Field field;
			field.setKey(QStringLiteral("username"));
			field.setRequired(true);
			field.setType(QXmppDataForm::Field::TextSingleField);
			newDataForm.fields().append(field);
		}

		if (!iq.password().isNull()) {
			QXmppDataForm::Field field;
			field.setKey(QStringLiteral("password"));
			field.setRequired(true);
			field.setType(QXmppDataForm::Field::TextPrivateField);
			newDataForm.fields().append(field);
		}

		if (!iq.email().isNull()) {
			QXmppDataForm::Field field;
			field.setKey(QStringLiteral("email"));
			field.setRequired(true);
			field.setType(QXmppDataForm::Field::TextPrivateField);
			newDataForm.fields().append(field);
		}
	} else {
		isFakeForm = false;
	}

	return newDataForm;
}

void RegistrationManager::copyUserDefinedValuesToNewForm(const QXmppDataForm &oldForm, QXmppDataForm& newForm)
{
	// Copy values from the last form.
	const QList<QXmppDataForm::Field> oldFields = oldForm.fields();
	for (const auto &field : oldFields) {
		// Only copy fields which:
		//  * are required,
		//  * are visible to the user
		//  * do not have a media element (e.g. CAPTCHA)
		if (field.isRequired() && field.type() != QXmppDataForm::Field::HiddenField &&
			field.mediaSources().isEmpty()) {
			for (auto &fieldFromNewForm : newForm.fields()) {
				if (fieldFromNewForm.key() == field.key()) {
					fieldFromNewForm.setValue(field.value());
					break;
				}
			}
		}
	}
}

void RegistrationManager::cleanUpLastForm()
{
	delete m_dataFormModel;

	// Remove content IDs from the last form.
	for (auto itr = m_contentIdsToRemove.begin(); itr != m_contentIdsToRemove.end();) {
		if (BitsOfBinaryImageProvider::instance()->removeImage(*itr)) {
			itr = m_contentIdsToRemove.erase(itr);
		} else {
			++itr;
		}
	}
}

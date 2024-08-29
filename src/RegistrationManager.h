// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
// Qt
#include <QObject>
#include <QTimer>
#include <QVector>
// QXmpp
#include <QXmppBitsOfBinaryContentId.h>
#include <QXmppStanza.h>

class ClientWorker;
class RegistrationDataFormModel;
class DataFormModel;
class QXmppRegistrationManager;
class QXmppClient;
class QXmppDataForm;
class QXmppRegisterIq;

class RegistrationManager : public QObject
{
	Q_OBJECT

public:
	enum RegistrationError {
		UnknownError,
		InBandRegistrationNotSupported,
		UsernameConflict,
		PasswordTooWeak,
		CaptchaVerificationFailed,
		RequiredInformationMissing,
		TemporarilyBlocked
	};
	Q_ENUM(RegistrationError)

	RegistrationManager(ClientWorker *clientWorker, QXmppClient *client, QObject *parent = nullptr);

	/**
	 * Connects to the server and requests a data form for account registration.
	 */
	Q_SIGNAL void registrationFormRequested();

	bool registerOnConnectEnabled() const;

	/**
	 * Emitted to send a completed data form for registration.
	 */
	Q_SIGNAL void sendRegistrationFormRequested();

	/**
	 * Aborts an ongoing registration.
	 */
	Q_SIGNAL void abortRegistrationRequested();

	/**
	 * Deletes the account from the server.
	 */
	void deleteAccount();

	Q_SIGNAL void changePasswordRequested(const QString &newPassword);

private:
	void requestRegistrationForm();

	/**
	 * Sets whether a registration is requested for the next time when the client connects to the server.
	 *
	 * @param registerOnConnect true for requesting a registration on connecting, false otherwise
	 */
	void setRegisterOnConnectEnabled(bool registerOnConnect);

	/**
	 * Sends the form containing information to register an account.
	 */
	void sendRegistrationForm();

	/**
	 * Changes the user's password.
	 *
	 * @param newPassword new password to set
	 */
	void changePassword(const QString &newPassword);

	/**
	 * Called when the In-Band Registration support changed.
	 *
	 * The server feature state for In-Band Registration is only changed
	 * when the server disables it, not on disconnect. That way, the last
	 * known state can be cached while being offline and operations like
	 * deleting an account from the server can be offered to the user even
	 * if Kaidan is not connected to the user's server.
	 */
	void handleInBandRegistrationSupportedChanged();

	/**
	 * Handles an incoming form used to register an account.
	 *
	 * @param iq IQ stanza to be handled
	 */
	void handleRegistrationFormReceived(const QXmppRegisterIq &iq);

	/**
	 * Handles a succeeded registration.
	 */
	void handleRegistrationSucceeded();

	/**
	 * Handles a failed registration.
	 *
	 * @param error error describing the reason for the failure
	 */
	void handleRegistrationFailed(const QXmppStanza::Error &error);

	/**
	 * Handles a successful password change.
	 *
	 * @param newPassword new password used to log in
	 */
	void handlePasswordChanged(const QString &newPassword);

	/**
	 * Handles a failed password change.
	 *
	 * @param error error describing the reason for the failure
	 */
	void handlePasswordChangeFailed(const QXmppStanza::Error &error);

	/**
	 * Extracts a form from an IQ stanza for registration.
	 *
	 * @param iq IQ stanza containing the form
	 * @param isFakeForm true if the form is used to TODO: what?
	 *
	 * @return data form with extracted key-value pairs
	 */
	QXmppDataForm extractFormFromRegisterIq(const QXmppRegisterIq &iq, bool &isFakeForm);

	/**
	 * Copies values set by the user to a new form.
	 *
	 * @param oldForm form with old values
	 * @param newForm form with new values
	 */
	void copyUserDefinedValuesToNewForm(const QXmppDataForm &oldForm, QXmppDataForm &newForm);

	/**
	 * Aborts an ongoing registration.
	 */
	void abortRegistration();

	/**
	 * Cleans up the last form used for registration.
	 */
	void cleanUpLastForm(RegistrationDataFormModel *newDataFormModel);

	ClientWorker *m_clientWorker;
	QXmppClient *m_client;
	QXmppRegistrationManager *m_manager;
	RegistrationDataFormModel *m_dataFormModel;
	QVector<QXmppBitsOfBinaryContentId> m_contentIdsToRemove;
};

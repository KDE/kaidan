// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QFuture>
#include <QObject>
// QXmpp
#include <QXmppBitsOfBinaryContentId.h>
#include <QXmppStanza.h>

class ClientWorker;
class DataFormModel;
class RegistrationDataFormModel;
class QXmppClient;
class QXmppDataForm;
class QXmppRegisterIq;
class QXmppRegistrationManager;

class RegistrationController : public QObject
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

    RegistrationController(QObject *parent = nullptr);

    QFuture<bool> registerOnConnectEnabled();

    /**
     * Connects to the server and requests a data form for account registration.
     */
    Q_INVOKABLE void requestRegistrationForm();

    /**
     * Emitted when a data form for registration is received from the server.
     *
     * @param dataFormModel received model for the registration data form
     */
    Q_SIGNAL void registrationFormReceived(DataFormModel *dataFormModel);

    /**
     * Emitted when an out-of-band URL for registration is received from the
     * server.
     *
     * @param outOfBandUrl URL used for out-of-band registration
     */
    Q_SIGNAL void registrationOutOfBandUrlReceived(const QUrl &outOfBandUrl);

    /**
     * Sends the completed data form containing information to register an account.
     */
    Q_INVOKABLE void sendRegistrationForm();

    /**
     * Emitted when the account registration failed.
     *
     * @param error received error
     * @param errorMessage message describing the error
     */
    Q_SIGNAL void registrationFailed(quint8 error, const QString &errorMessage);

    /**
     * Changes the user's password.
     *
     * @param newPassword new password to set
     */
    Q_INVOKABLE void changePassword(const QString &newPassword);

    /**
     * Emitted when changing of the user's password finished succfessully.
     */
    Q_SIGNAL void passwordChangeSucceeded();

    /**
     * Emitted when changing the user's password failed.
     *
     * @param errorMessage message describing the error
     */
    Q_SIGNAL void passwordChangeFailed(const QString &errorMessage);

    /**
     * Deletes the account from the server.
     */
    void deleteAccount();

private:
    /**
     * Sets whether a registration is requested for the next time when the client connects to the server.
     *
     * @param registerOnConnect true for requesting a registration on connecting, false otherwise
     */
    void setRegisterOnConnectEnabled(bool registerOnConnect);

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
    static QXmppDataForm extractFormFromRegisterIq(const QXmppRegisterIq &iq, bool &isFakeForm);

    /**
     * Aborts an ongoing registration.
     */
    void abortRegistration();

    /**
     * Updates the last form used for registration.
     */
    void updateLastForm(const QXmppDataForm &newDataForm);

    /**
     * Cleans up the last form used for registration.
     */
    void cleanUpLastForm();

    /**
     * Copies values set by the user to a new form.
     *
     * @param oldForm form with old values
     * @param newForm form with new values
     */
    static void copyUserDefinedValuesToNewForm(const QXmppDataForm &oldForm, const QXmppDataForm &newForm);

    /**
     * Removes content IDs from the last form.
     */
    void removeOldContentIds();

    ClientWorker *const m_clientWorker;
    QXmppClient *const m_client;
    QXmppRegistrationManager *const m_manager;
    RegistrationDataFormModel *m_dataFormModel = nullptr;
    QList<QXmppBitsOfBinaryContentId> m_contentIdsToRemove;
};

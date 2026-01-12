// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RegistrationController.h"

// std
#include <chrono>
// Qt
#include <QTimer>
// QXmpp
#include <QXmppBitsOfBinaryDataList.h>
#include <QXmppRegistrationManager.h>
// Kaidan
#include "AccountController.h"
#include "AccountDb.h"
#include "ClientController.h"
#include "EncryptionController.h"
#include "ImageProvider.h"
#include "MessageDb.h"
#include "RegistrationDataFormModel.h"
#include "RosterDb.h"
#include "VCardController.h"

using namespace std::chrono_literals;

constexpr auto ACCOUNT_DELETION_TIMEOUT = 5s;

RegistrationController::RegistrationController(AccountSettings *accountSettings,
                                               Connection *connection,
                                               EncryptionController *encryptionController,
                                               VCardController *vCardController,
                                               ClientController *clientController,
                                               QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_connection(connection)
    , m_encryptionController(encryptionController)
    , m_vCardController(vCardController)
    , m_clientController(clientController)
    , m_client(clientController->xmppClient())
    , m_manager(clientController->registrationManager())
{
    connect(m_manager, &QXmppRegistrationManager::supportedByServerChanged, this, &RegistrationController::handleInBandRegistrationSupportedChanged);

    // account creation
    connect(m_manager, &QXmppRegistrationManager::registrationFormReceived, this, &RegistrationController::handleRegistrationFormReceived);
    connect(m_manager, &QXmppRegistrationManager::registrationSucceeded, this, &RegistrationController::handleRegistrationSucceeded);
    connect(m_manager, &QXmppRegistrationManager::registrationFailed, this, &RegistrationController::handleRegistrationFailed);

    // account deletion
    connect(m_manager, &QXmppRegistrationManager::accountDeleted, this, &RegistrationController::handleAccountDeletedFromServer);
    connect(m_manager, &QXmppRegistrationManager::accountDeletionFailed, this, &RegistrationController::handleAccountDeletionFromServerFailed);

    // password change
    connect(m_manager, &QXmppRegistrationManager::passwordChanged, this, &RegistrationController::handlePasswordChanged);
    connect(m_manager, &QXmppRegistrationManager::passwordChangeFailed, this, &RegistrationController::handlePasswordChangeFailed);
}

bool RegistrationController::registerOnConnectEnabled() const
{
    return m_manager->registerOnConnectEnabled();
}

void RegistrationController::requestRegistrationForm()
{
    if (m_connection->state() != Enums::ConnectionState::StateDisconnected) {
        if (m_manager->registerOnConnectEnabled()) {
            if (m_client->configuration().jidBare() == m_accountSettings->jid()) {
                m_manager->requestRegistrationForm();
            } else {
                m_client->disconnectFromServer();
                m_clientController->connectToServer();
            }
        } else {
            m_client->disconnectFromServer();
            m_manager->setRegisterOnConnectEnabled(true);
            m_clientController->connectToServer();
        }
    } else {
        m_manager->setRegisterOnConnectEnabled(true);
        m_clientController->connectToServer();
    }
}

void RegistrationController::sendRegistrationForm()
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

    // If the client is still connected/connecting to the server, send the registration form
    // directly.
    // Otherwise, reconnect to the server beforehand.
    if (m_connection->state() != Enums::ConnectionState::StateDisconnected) {
        m_manager->sendCachedRegistrationForm();
    } else {
        m_clientController->connectToServer();
    }
}

void RegistrationController::abortRegistration()
{
    // Reset the registration steps if the client connected to a server for registration.
    if (registerOnConnectEnabled()) {
        // Disconnect from any server that the client is still connected to.
        if (m_connection->state() != Enums::ConnectionState::StateDisconnected) {
            m_client->disconnectFromServer();
        }

        cleanUpLastForm();
        setRegisterOnConnectEnabled(false);
    }
}

void RegistrationController::changePassword(const QString &newPassword)
{
    m_manager->changePassword(newPassword);
}

void RegistrationController::deleteAccountFromClient()
{
    m_deletionStates = DeletionState::ToBeDeletedFromClient;

    // If the client is not yet disconnected, disconnect first and delete the account afterwards.
    // Otherwise, delete the account directly from the client.
    if (m_clientController->xmppClient()->isAuthenticated()) {
        m_encryptionController->reset().then([this]() {
            m_clientController->logOut();
        });
    } else {
        m_connection->logIn();
    }
}

void RegistrationController::deleteAccountFromClientAndServer()
{
    m_deletionStates = DeletionState::ToBeDeletedFromClient | DeletionState::ToBeDeletedFromServer;

    // If the client is already connected, delete the account directly from the server.
    // Otherwise, connect first and delete the account afterwards.
    if (m_clientController->xmppClient()->isAuthenticated()) {
        m_manager->deleteAccount();

        // Start a timer to disconnect from the server after a specified timeout triggering
        // QXmppRegistrationManager::accountDeleted to be emitted.
        // Considering an account as deleted after a timeout is needed because some servers do not
        // respond to the deletion request within a reasonable time period.
        QTimer::singleShot(ACCOUNT_DELETION_TIMEOUT, m_client, &QXmppClient::disconnectFromServer);
    } else {
        m_deletionStates |= DeletionState::ClientDisconnectedBeforeDeletionFromServer;
        m_connection->logIn();
    }
}

bool RegistrationController::handleConnected()
{
    // If the account could not be deleted because the client was disconnected, delete it now.
    if (m_deletionStates.testFlags(DeletionState::ToBeDeletedFromClient | DeletionState::ToBeDeletedFromServer)) {
        deleteAccountFromClientAndServer();

        return true;
    } else if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient)) {
        deleteAccountFromClient();
        return true;
    }

    return false;
}

void RegistrationController::handleDisconnected()
{
    // Delete the account from the client if the account was deleted from the server or the client
    // was connected and had to disconnect first.
    if (m_deletionStates.testFlag(DeletionState::DeletedFromServer)) {
        m_encryptionController->resetLocally().then(this, [this]() {
            removeLocalAccountData();
        });
    } else if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient)) {
        removeLocalAccountData();
    }

    m_deletionStates = DeletionState::NotToBeDeleted;
}

void RegistrationController::setRegisterOnConnectEnabled(bool registerOnConnect)
{
    m_manager->setRegisterOnConnectEnabled(registerOnConnect);
}

void RegistrationController::handleInBandRegistrationSupportedChanged()
{
    if (m_connection->state() == Enums::ConnectionState::StateConnected) {
        m_accountSettings->setInBandRegistrationFeaturesSupported(m_manager->supportedByServer());
    }
}

void RegistrationController::handleRegistrationFormReceived(const QXmppRegisterIq &iq)
{
    bool isFakeForm;
    QXmppDataForm newDataForm = extractFormFromRegisterIq(iq, isFakeForm);

    // If there is no registration data form, try to use an out-of-band URL.
    if (newDataForm.constFields().isEmpty()) {
        // If there is a standardized out-of-band URL, use that.
        if (!iq.outOfBandUrl().isEmpty()) {
            Q_EMIT registrationOutOfBandUrlReceived(QUrl{iq.outOfBandUrl()});
            abortRegistration();
            return;
        }

        // Try to find an out-of-band URL within the instructions element.
        // Most servers include a text with a link to the website.
        const auto words = iq.instructions().split(u' ');
        for (const auto &instructionPart : words) {
            if (instructionPart.startsWith(u"https://")) {
                Q_EMIT registrationOutOfBandUrlReceived(QUrl{instructionPart});
                abortRegistration();
                return;
            }
        }

        // If no URL has been found in the instructions, there is a problem with the server.
        Q_EMIT m_clientController->connectionErrorChanged(ClientController::RegistrationUnsupported);
        abortRegistration();
        return;
    }

    if (m_dataFormModel) {
        updateLastForm(newDataForm);
    } else {
        m_dataFormModel = new RegistrationDataFormModel(newDataForm);
    }

    m_dataFormModel->setIsFakeForm(isFakeForm);

    // Add the attached Bits of Binary data to the corresponding image provider.
    const auto bobDataList = iq.bitsOfBinaryData();
    for (const auto &bobData : bobDataList) {
        ImageProvider::instance()->addImage(bobData);
        m_contentIdsToRemove << bobData.cid();
    }

    Q_EMIT registrationFormReceived(m_dataFormModel);
}

void RegistrationController::handleRegistrationSucceeded()
{
    m_accountSettings->setJid(m_dataFormModel->extractUsername().append(QLatin1Char('@')).append(m_client->configuration().domain()));
    m_accountSettings->setPassword(m_dataFormModel->extractPassword());
    m_accountSettings->storeTemporaryData();

    m_client->disconnectFromServer();
    m_manager->setRegisterOnConnectEnabled(false);

    connect(
        m_connection,
        &Connection::connected,
        this,
        [this]() {
            m_vCardController->changeNickname(m_accountSettings->name());
        },
        Qt::SingleShotConnection);

    m_clientController->logIn();
    cleanUpLastForm();
}

void RegistrationController::handleRegistrationFailed(const QXmppStanza::Error &error)
{
    RegistrationError registrationError = RegistrationError::UnknownError;

    switch (error.type()) {
    case QXmppStanza::Error::Cancel:
        if (error.condition() == QXmppStanza::Error::FeatureNotImplemented) {
            registrationError = RegistrationError::InBandRegistrationNotSupported;
            abortRegistration();
        } else if (error.condition() == QXmppStanza::Error::Conflict) {
            registrationError = RegistrationError::UsernameConflict;
        } else if (error.condition() == QXmppStanza::Error::NotAllowed && error.text().contains(QStringLiteral("captcha"), Qt::CaseInsensitive)) {
            registrationError = RegistrationError::CaptchaVerificationFailed;
        }

        break;
    case QXmppStanza::Error::Modify:
        if (error.condition() == QXmppStanza::Error::NotAcceptable) {
            // TODO: Check error text in English (needs QXmpp change)
            if (error.text().contains(QStringLiteral("password"), Qt::CaseInsensitive)
                && (error.text().contains(QStringLiteral("weak"), Qt::CaseInsensitive)
                    || error.text().contains(QStringLiteral("short"), Qt::CaseInsensitive))) {
                registrationError = RegistrationError::PasswordTooWeak;
            } else if (error.text().contains(QStringLiteral("ip"), Qt::CaseInsensitive)
                       || error.text().contains(QStringLiteral("quickly"), Qt::CaseInsensitive)) {
                registrationError = RegistrationError::TemporarilyBlocked;
                abortRegistration();
            } else {
                registrationError = RegistrationError::RequiredInformationMissing;
            }
        } else if (error.condition() == QXmppStanza::Error::BadRequest && error.text().contains(QStringLiteral("captcha"), Qt::CaseInsensitive)) {
            registrationError = RegistrationError::CaptchaVerificationFailed;
        }

        break;
    case QXmppStanza::Error::Auth:
        if (error.condition() == QXmppStanza::Error::Forbidden) {
            registrationError = RegistrationError::InBandRegistrationNotSupported;
            abortRegistration();
        }
        break;
    case QXmppStanza::Error::Wait:
        if (error.condition() == QXmppStanza::Error::ResourceConstraint || error.condition() == QXmppStanza::Error::PolicyViolation) {
            registrationError = RegistrationError::TemporarilyBlocked;
            abortRegistration();
        }
        break;
    default:
        break;
    }

    Q_EMIT registrationFailed(registrationError, error.text());
}

void RegistrationController::handlePasswordChanged(const QString &newPassword)
{
    m_accountSettings->setPassword(newPassword);
    m_accountSettings->storeTemporaryData();
    Q_EMIT passwordChangeSucceeded();
}

void RegistrationController::handlePasswordChangeFailed(const QXmppStanza::Error &error)
{
    Q_EMIT passwordChangeFailed(error.text());
}

QXmppDataForm RegistrationController::extractFormFromRegisterIq(const QXmppRegisterIq &iq, bool &isFakeForm)
{
    QXmppDataForm newDataForm = iq.form();

    if (newDataForm.constFields().isEmpty()) {
        // This is a hack, so we only need to implement one way of registering in QML.
        // A 'fake' data form model is created with a username and password field.
        isFakeForm = true;

        if (!iq.username().isNull()) {
            QXmppDataForm::Field field;
            field.setKey(QStringLiteral("username"));
            field.setRequired(true);
            field.setType(QXmppDataForm::Field::TextSingleField);
            newDataForm.appendField(std::move(field));
        }

        if (!iq.password().isNull()) {
            QXmppDataForm::Field field;
            field.setKey(QStringLiteral("password"));
            field.setRequired(true);
            field.setType(QXmppDataForm::Field::TextPrivateField);
            newDataForm.appendField(std::move(field));
        }

        if (!iq.email().isNull()) {
            QXmppDataForm::Field field;
            field.setKey(QStringLiteral("email"));
            field.setRequired(true);
            field.setType(QXmppDataForm::Field::TextPrivateField);
            newDataForm.appendField(std::move(field));
        }
    } else {
        isFakeForm = false;
    }

    return newDataForm;
}

void RegistrationController::updateLastForm(QXmppDataForm &newDataForm)
{
    copyUserDefinedValuesToNewForm(m_dataFormModel->form(), newDataForm);
    m_dataFormModel = new RegistrationDataFormModel(newDataForm);
    removeOldContentIds();
}

void RegistrationController::cleanUpLastForm()
{
    m_dataFormModel = nullptr;
    removeOldContentIds();
}

void RegistrationController::copyUserDefinedValuesToNewForm(const QXmppDataForm &oldForm, QXmppDataForm &newForm)
{
    for (const auto &field : oldForm.constFields()) {
        // Only copy fields that:
        //  * are required
        //  * are visible to the user
        //  * do not have a media element (e.g., a CAPTCHA)
        if (field.isRequired() && field.type() != QXmppDataForm::Field::HiddenField && field.mediaSources().isEmpty()) {
            auto newFormFields = newForm.fields();

            for (auto &newFormField : newFormFields) {
                if (newFormField.key() == field.key()) {
                    newFormField.setValue(field.value());
                    break;
                }
            }

            newForm.setFields(newFormFields);
        }
    }
}

void RegistrationController::removeOldContentIds()
{
    for (auto itr = m_contentIdsToRemove.begin(); itr != m_contentIdsToRemove.end();) {
        if (ImageProvider::instance()->removeImage(*itr)) {
            itr = m_contentIdsToRemove.erase(itr);
        } else {
            ++itr;
        }
    }
}

void RegistrationController::handleAccountDeletedFromServer()
{
    m_deletionStates = DeletionState::DeletedFromServer;
}

void RegistrationController::handleAccountDeletionFromServerFailed(const QXmppStanza::Error &error)
{
    Q_EMIT accountDeletionFromClientAndServerFailed(error.text());

    if (m_deletionStates.testFlag(DeletionState::ClientDisconnectedBeforeDeletionFromServer)) {
        m_deletionStates = DeletionState::NotToBeDeleted;
        m_clientController->logOut();
    } else {
        m_deletionStates = DeletionState::NotToBeDeleted;
    }
}

void RegistrationController::removeLocalAccountData()
{
    MessageDb::instance()->removeMessages(m_accountSettings->jid());
    RosterDb::instance()->removeItems(m_accountSettings->jid());
    AccountDb::instance()->removeAccount(m_accountSettings->jid());
}

#include "moc_RegistrationController.cpp"

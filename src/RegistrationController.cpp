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
#include "BitsOfBinaryImageProvider.h"
#include "ClientWorker.h"
#include "FutureUtils.h"
#include "RegistrationDataFormModel.h"
#include "ServerFeaturesCache.h"

using namespace std::chrono_literals;

constexpr auto ACCOUNT_DELETION_TIMEOUT = 5s;

RegistrationController::RegistrationController(QObject *parent)
    : QObject(parent)
    , m_clientWorker(Kaidan::instance()->client())
    , m_client(m_clientWorker->xmppClient())
    , m_manager(m_clientWorker->registrationManager())
{
    connect(m_manager, &QXmppRegistrationManager::supportedByServerChanged, this, &RegistrationController::handleInBandRegistrationSupportedChanged);

    // account creation
    connect(m_manager, &QXmppRegistrationManager::registrationFormReceived, this, &RegistrationController::handleRegistrationFormReceived);
    connect(m_manager, &QXmppRegistrationManager::registrationSucceeded, this, &RegistrationController::handleRegistrationSucceeded);
    connect(m_manager, &QXmppRegistrationManager::registrationFailed, this, &RegistrationController::handleRegistrationFailed);

    // account deletion
    connect(m_manager, &QXmppRegistrationManager::accountDeleted, AccountController::instance(), &AccountController::handleAccountDeletedFromServer);
    connect(m_manager,
            &QXmppRegistrationManager::accountDeletionFailed,
            AccountController::instance(),
            &AccountController::handleAccountDeletionFromServerFailed);

    // password change
    connect(m_manager, &QXmppRegistrationManager::passwordChanged, this, &RegistrationController::handlePasswordChanged);
    connect(m_manager, &QXmppRegistrationManager::passwordChangeFailed, this, &RegistrationController::handlePasswordChangeFailed);
}

QFuture<bool> RegistrationController::registerOnConnectEnabled()
{
    return runAsync(m_manager, [this]() {
        return m_manager->registerOnConnectEnabled();
    });
}

void RegistrationController::requestRegistrationForm()
{
    auto connectToRegister = [this]() {
        setRegisterOnConnectEnabled(true);

        runOnThread(m_clientWorker, [this]() {
            m_clientWorker->connectToServer();
        });
    };

    auto reconnectToRegister = [this, connectToRegister]() {
        m_client->disconnectFromServer();
        connectToRegister();
    };

    if (Kaidan::instance()->connectionState() != Enums::ConnectionState::StateDisconnected) {
        registerOnConnectEnabled().then(this, [this, reconnectToRegister](bool registerOnConnectEnabled) {
            if (registerOnConnectEnabled) {
                runOnThread(
                    m_client,
                    [this]() {
                        return m_client->configuration().jidBare();
                    },
                    this,
                    [this, reconnectToRegister](QString &&accountJid) {
                        if (accountJid == AccountController::instance()->account().jid) {
                            m_manager->requestRegistrationForm();
                        } else {
                            reconnectToRegister();
                        }
                    });
            } else {
                reconnectToRegister();
            }
        });
    } else {
        connectToRegister();
    }
}

void RegistrationController::sendRegistrationForm()
{
    if (m_dataFormModel->isFakeForm()) {
        QXmppRegisterIq iq;
        iq.setUsername(m_dataFormModel->extractUsername());
        iq.setPassword(m_dataFormModel->extractPassword());
        iq.setEmail(m_dataFormModel->extractEmail());

        runOnThread(m_manager, [this, iq]() {
            m_manager->setRegistrationFormToSend(iq);
        });
    } else {
        runOnThread(m_manager, [this, iq = m_dataFormModel->form()]() {
            m_manager->setRegistrationFormToSend(iq);
        });
    }

    setRegisterOnConnectEnabled(true);

    // If the client is still connected/connecting to the server, send the registration form
    // directly.
    // Otherwise, reconnect to the server beforehand.
    if (Kaidan::instance()->connectionState() != Enums::ConnectionState::StateDisconnected) {
        runOnThread(m_manager, [this]() {
            m_manager->sendCachedRegistrationForm();
        });
    } else {
        runOnThread(m_clientWorker, [this]() {
            m_clientWorker->connectToServer();
        });
    }
}

void RegistrationController::changePassword(const QString &newPassword)
{
    runOnThread(m_clientWorker, [this, newPassword]() {
        m_clientWorker->startTask([this, newPassword] {
            m_manager->changePassword(newPassword);
        });
    });
}

void RegistrationController::deleteAccount()
{
    runOnThread(m_manager, [this]() {
        m_manager->deleteAccount();
    });

    // Start a timer to disconnect from the server after a specified timeout triggering
    // QXmppRegistrationManager::accountDeleted to be emitted.
    // Considering an account as deleted after a timeout is needed because some servers do not
    // respond to the deletion request within a reasonable time period.
    QTimer::singleShot(ACCOUNT_DELETION_TIMEOUT, m_client, &QXmppClient::disconnectFromServer);
}

void RegistrationController::setRegisterOnConnectEnabled(bool registerOnConnect)
{
    runOnThread(m_manager, [this, registerOnConnect]() {
        m_manager->setRegisterOnConnectEnabled(registerOnConnect);
    });
}

void RegistrationController::handleInBandRegistrationSupportedChanged()
{
    if (Kaidan::instance()->connectionState() == Enums::ConnectionState::StateConnected) {
        runOnThread(
            m_manager,
            [this]() {
                return m_manager->supportedByServer();
            },
            this,
            [](bool supportedByServer) {
                Kaidan::instance()->serverFeaturesCache()->setInBandRegistrationSupported(supportedByServer);
            });
    }
}

void RegistrationController::handleRegistrationFormReceived(const QXmppRegisterIq &iq)
{
    bool isFakeForm;
    QXmppDataForm newDataForm = extractFormFromRegisterIq(iq, isFakeForm);

    // If there is no registration data form, try to use an out-of-band URL.
    if (newDataForm.fields().isEmpty()) {
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
        Q_EMIT m_clientWorker->connectionErrorChanged(ClientWorker::RegistrationUnsupported);
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
        BitsOfBinaryImageProvider::instance()->addImage(bobData);
        m_contentIdsToRemove << bobData.cid();
    }

    Q_EMIT registrationFormReceived(m_dataFormModel);
}

void RegistrationController::handleRegistrationSucceeded()
{
    AccountController::instance()->setNewAccount(m_dataFormModel->extractUsername().append(QLatin1Char('@')).append(m_client->configuration().domain()),
                                                 m_dataFormModel->extractPassword());

    runOnThread(m_client, [this]() {
        m_client->disconnectFromServer();
    });

    setRegisterOnConnectEnabled(false);

    runOnThread(m_clientWorker, [this]() {
        m_clientWorker->logIn();
    });

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

    Q_EMIT registrationFailed(quint8(registrationError), error.text());
}

void RegistrationController::handlePasswordChanged(const QString &newPassword)
{
    AccountController::instance()->setAuthPassword(newPassword);

    runOnThread(m_clientWorker, [this]() {
        m_clientWorker->finishTask();
    });

    Q_EMIT passwordChangeSucceeded();
}

void RegistrationController::handlePasswordChangeFailed(const QXmppStanza::Error &error)
{
    Q_EMIT passwordChangeFailed(error.text());

    runOnThread(m_clientWorker, [this]() {
        m_clientWorker->finishTask();
    });
}

QXmppDataForm RegistrationController::extractFormFromRegisterIq(const QXmppRegisterIq &iq, bool &isFakeForm)
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

void RegistrationController::abortRegistration()
{
    const auto accountController = AccountController::instance();

    // Reset the registration steps if the client connected to a server for registration.
    if (accountController->hasNewAccount()) {
        // Resetting the cached JID is needed to clear the JID field of LoginArea.
        accountController->resetNewAccount();

        // Disconnect from any server that the client is still connected to.
        if (Kaidan::instance()->connectionState() != Enums::ConnectionState::StateDisconnected) {
            runOnThread(m_client, [this]() {
                m_client->disconnectFromServer();
            });
        }

        setRegisterOnConnectEnabled(false);
        cleanUpLastForm();
    }
}

void RegistrationController::updateLastForm(const QXmppDataForm &newDataForm)
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

void RegistrationController::copyUserDefinedValuesToNewForm(const QXmppDataForm &oldForm, const QXmppDataForm &newForm)
{
    // Copy values from the last form.
    const QList<QXmppDataForm::Field> oldFields = oldForm.fields();
    for (const auto &field : oldFields) {
        // Only copy fields which:
        //  * are required
        //  * are visible to the user
        //  * do not have a media element (e.g., a CAPTCHA)
        if (field.isRequired() && field.type() != QXmppDataForm::Field::HiddenField && field.mediaSources().isEmpty()) {
            for (auto &fieldFromNewForm : newForm.fields()) {
                if (fieldFromNewForm.key() == field.key()) {
                    fieldFromNewForm.setValue(field.value());
                    break;
                }
            }
        }
    }
}

void RegistrationController::removeOldContentIds()
{
    for (auto itr = m_contentIdsToRemove.begin(); itr != m_contentIdsToRemove.end();) {
        if (BitsOfBinaryImageProvider::instance()->removeImage(*itr)) {
            itr = m_contentIdsToRemove.erase(itr);
        } else {
            ++itr;
        }
    }
}

#include "moc_RegistrationController.cpp"

// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Keychain.h"

// Qt
#include <QAtomicInteger>
#include <QDir>
#include <QStandardPaths>
// Kaidan
#include "MainController.h"

const auto FLATPAK_VARIABLE = "container";
const auto FLATPAK_SERVICE_KEY_PART = QStringLiteral("Flatpak");

const auto UNENCRYPTED_KEYCHAIN_FORMAT = QSettings::IniFormat;
const auto UNENCRYPTED_KEYCHAIN_FILENAME = QStringLiteral("keychain.ini");

namespace QKeychainFuture
{
QAtomicInteger<ushort> &unencryptedFallbackAtomic()
{
    static QAtomicInteger<ushort> unencryptedFallback;
    return unencryptedFallback;
}

QString unencryptedKeychainFilePath()
{
    // Check if there is a writable location for app data.
    const auto appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        qFatal("Could not find writable location for unencrypted keychain file");
    }

    const auto appDataDirectory = QDir(appDataPath);
    if (!appDataDirectory.mkpath(QLatin1String("."))) {
        qFatal("Could not create writable directory %s", qPrintable(appDataDirectory.absolutePath()));
    }

    // Create the absolute unencrypted keychain file path while ensuring that there is a writable
    // location on all systems.
    const auto unencryptedKeychainFilePath = appDataDirectory.absoluteFilePath(UNENCRYPTED_KEYCHAIN_FILENAME);

    return unencryptedKeychainFilePath;
}
}

// Error

QKeychainFuture::Error::Error(QKeychain::Error error, const QString &message)
    : m_error(error)
    , m_message(message.toLocal8Bit())
{
}

QKeychainFuture::Error::~Error() = default;

const char *QKeychainFuture::Error::what() const noexcept
{
    return m_message.constData();
}

void QKeychainFuture::Error::raise() const
{
    throw *this;
}

QKeychainFuture::Error *QKeychainFuture::Error::clone() const
{
    return new Error(*this);
}

QKeychain::Error QKeychainFuture::Error::error() const
{
    return m_error;
}

QByteArray QKeychainFuture::Error::message() const
{
    return m_message;
}

// APIs

QKeychainFuture::DeleteFuture QKeychainFuture::deleteServiceKey(const QString &service, const QString &key)
{
    QPromise<QFutureValueType<DeleteFuture>> promise;
    auto future = promise.future();

    if (QKeychainFuture::unencryptedFallback()) {
        auto settings = QKeychainFuture::unencryptedSettings();

        settings->beginGroup(service);
        settings->remove(key);
        settings->endGroup();
        settings->sync();

        if (settings->status() == QSettings::NoError) {
            promise.addResult(QKeychain::NoError);
        } else {
            promise.setException(Error(QKeychain::CouldNotDeleteEntry, QStringLiteral("Could not delete entry")));
            promise.addResult(QKeychain::CouldNotDeleteEntry);
        }

        promise.finish();

        return future;
    }

    QMetaObject::invokeMethod(qApp, [service, key, promise = std::move(promise)]() mutable {
        auto job = new QKeychain::DeletePasswordJob(service);

        job->setKey(key);

        promise.start();

        QObject::connect(job, &QKeychain::Job::finished, job, [job, promise = std::move(promise)]() mutable {
            if (job->error() != QKeychain::NoError) {
                promise.setException(Error(job->error(), job->errorString()));
            }

            promise.addResult(job->error());
            promise.finish();
        });

        job->start();
    });

    return future;
}

QKeychainFuture::DeleteFuture QKeychainFuture::deleteService(const QString &service)
{
    return deleteServiceKey(service, {});
}

QKeychainFuture::DeleteFuture QKeychainFuture::deleteKey(const QString &key)
{
    return deleteServiceKey(QKeychainFuture::serviceKey(), key);
}

bool QKeychainFuture::unencryptedFallback()
{
    return unencryptedFallbackAtomic() > 0;
}

void QKeychainFuture::setUnencryptedFallback(bool unencryptedFallback)
{
    unencryptedFallbackAtomic() = unencryptedFallback ? 1 : 0;
}

QString QKeychainFuture::serviceKey()
{
    QStringList serviceKeyParts = {QGuiApplication::applicationDisplayName()};

    if (!qEnvironmentVariableIsEmpty(FLATPAK_VARIABLE)) {
        serviceKeyParts.append(FLATPAK_SERVICE_KEY_PART);
    }

    if (static const auto profile = applicationProfile(); !profile.isEmpty()) {
        serviceKeyParts.append(profile);
    }

    return serviceKeyParts.join(QLatin1Char(' '));
}

std::unique_ptr<QSettings> QKeychainFuture::unencryptedSettings()
{
    return std::make_unique<QSettings>(unencryptedKeychainFilePath(), UNENCRYPTED_KEYCHAIN_FORMAT);
}

QFuture<QKeychain::Error> QKeychainFuture::migrateServiceToEncryptedKeychain(const QString &service)
{
    if (QKeychainFuture::unencryptedFallback()) {
        return QtFuture::makeReadyValueFuture(QKeychain::NoError);
    }

    auto settings = QKeychainFuture::unencryptedSettings();
    settings->beginGroup(service);

    auto keys = settings->allKeys();
    QList<WriteFuture> futures;

    for (const auto &key : std::as_const(keys)) {
        const auto value = settings->value(key);

        if (value.userType() == qMetaTypeId<QString>()) {
            futures.append(writeServiceKey(service, key, value.toString()));
        } else {
            futures.append(writeServiceKey(service, key, value.toByteArray()));
        }
    }

    return QtFuture::whenAll(futures.begin(), futures.end()).then([settings = std::move(settings), keys = std::move(keys)](const QList<WriteFuture> &results) {
        Q_ASSERT(keys.size() == results.size());
        std::optional<QKeychain::Error> error;

        for (int i = 0; i < results.size(); ++i) {
            const auto &future = results[i];

            if (future.isCanceled()) {
                if (!error) {
                    error = QKeychain::CouldNotDeleteEntry;
                }
            } else {
                if (const auto futureError = future.result(); futureError == QKeychain::NoError) {
                    const auto &key = keys[i];
                    settings->remove(key);
                } else {
                    if (!error) {
                        error = futureError;
                    }
                }
            }
        }

        settings->endGroup();
        settings->sync();

        if (settings->status() != QSettings::NoError) {
            throw Error(QKeychain::Error::CouldNotDeleteEntry, QStringLiteral("Could not remove password from unencrypted keychain file"));

            if (!error) {
                error = QKeychain::CouldNotDeleteEntry;
            }
        }

        return error.value_or(QKeychain::NoError);
    });
}

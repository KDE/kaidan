// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Keychain.h"

// Qt
#include <QAtomicInteger>
// Kaidan
#include "MainController.h"

const auto FLATPAK_VARIABLE = "container";
const auto FLATPAK_SERVICE_KEY_PART = QStringLiteral("Flatpak");

namespace QKeychainFuture
{
QAtomicInteger<ushort> &insecureFallbackAtomic()
{
    static QAtomicInteger<ushort> insecureFallback;
    return insecureFallback;
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

    QMetaObject::invokeMethod(qApp, [service, key, promise = std::move(promise)]() mutable {
        auto job = new QKeychain::DeletePasswordJob(service);

        job->setInsecureFallback(QKeychainFuture::insecureFallback());
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

bool QKeychainFuture::insecureFallback()
{
    return insecureFallbackAtomic() > 0;
}

void QKeychainFuture::setInsecureFallback(bool insecureFallback)
{
    insecureFallbackAtomic() = insecureFallback ? 1 : 0;
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

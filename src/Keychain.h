// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <chrono>
#include <variant>
// Qt
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QException>
#include <QFuture>
#include <qt6keychain/keychain.h>
// Kaidan
#include "FutureUtils.h"

using namespace std::chrono;

namespace QKeychainFuture
{
template<typename T>
using ReadResult = std::variant<T, QKeychain::Error>;
using WriteResult = QKeychain::Error;
using DeleteResult = WriteResult;

template<typename T>
using ReadFuture = QFuture<ReadResult<T>>;
using WriteFuture = QFuture<WriteResult>;
using DeleteFuture = QFuture<DeleteResult>;

// Helpers

bool insecureFallback();
void setInsecureFallback(bool insecureFallback);

template<typename T>
bool waitForFinished(const QFuture<T> &future, std::chrono::milliseconds msecs = -1ms)
{
    QDeadlineTimer deadline(msecs);

    do {
        QCoreApplication::processEvents();
    } while (!future.isFinished() && !deadline.hasExpired());

    return future.isFinished();
}

// Exceptions

class Error : public QException
{
public:
    explicit Error(QKeychain::Error error, const QString &message);
    ~Error() override;

    const char *what() const noexcept override;
    void raise() const override;
    Error *clone() const override;
    QKeychain::Error error() const;
    QByteArray message() const;

private:
    QKeychain::Error m_error;
    QByteArray m_message;
};

// Read

template<typename T>
ReadFuture<T> readServiceKey(const QString &service, const QString &key)
{
    QPromise<QFutureValueType<ReadFuture<T>>> promise;
    auto future = promise.future();
    auto job = new QKeychain::ReadPasswordJob(service);

    job->setInsecureFallback(QKeychainFuture::insecureFallback());
    job->setKey(key);

    promise.start();

    QObject::connect(job, &QKeychain::Job::finished, job, [job, promise = std::move(promise)]() mutable {
        if (job->error() != QKeychain::NoError) {
            promise.setException(Error(job->error(), job->errorString()));
            promise.addResult(job->error());
        } else {
            if constexpr (std::is_same_v<T, QString>) {
                promise.addResult(job->textData());
            } else if constexpr (std::is_same_v<T, QByteArray>) {
                promise.addResult(job->binaryData());
            }
        }

        promise.finish();
    });

    job->start();

    return future;
}

template<typename T>
ReadFuture<T> readService(const QString &service)
{
    return readServiceKey<T>(service, {});
}

template<typename T>
ReadFuture<T> readKey(const QString &key)
{
    return readServiceKey<T>(QCoreApplication::applicationName(), key);
}

// Write

template<typename T>
WriteFuture writeServiceKey(const QString &service, const QString &key, const T &value)
{
    QPromise<QFutureValueType<WriteFuture>> promise;
    auto future = promise.future();
    auto job = new QKeychain::WritePasswordJob(service);

    job->setInsecureFallback(QKeychainFuture::insecureFallback());
    job->setKey(key);

    if constexpr (std::is_same_v<T, QString>) {
        job->setTextData(value);
    } else if constexpr (std::is_same_v<T, QByteArray>) {
        job->setBinaryData(value);
    }

    promise.start();

    QObject::connect(job, &QKeychain::Job::finished, job, [job, promise = std::move(promise)]() mutable {
        if (job->error() != QKeychain::NoError) {
            promise.setException(Error(job->error(), job->errorString()));
        }

        promise.addResult(job->error());
        promise.finish();
    });

    job->start();

    return future;
}

template<typename T>
WriteFuture writeService(const QString &service, const T &value)
{
    return writeServiceKey(service, {}, value);
}

template<typename T>
WriteFuture writeKey(const QString &key, const T &value)
{
    return writeServiceKey(QCoreApplication::applicationName(), key, value);
}

// Delete

DeleteFuture deleteServiceKey(const QString &service, const QString &key);
DeleteFuture deleteService(const QString &service);
DeleteFuture deleteKey(const QString &key);
} // QKeychainFuture

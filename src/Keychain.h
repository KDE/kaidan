// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <chrono>
#include <memory>
#include <variant>
// Qt
#include <QDeadlineTimer>
#include <QException>
#include <QFuture>
#include <QGuiApplication>
#include <QSettings>
#include <qt6keychain/keychain.h>

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

template<typename ValueType>
auto qFutureValueType(QFuture<ValueType>) -> ValueType;
template<typename Future>
using QFutureValueType = decltype(qFutureValueType(Future()));

// Helpers

bool unencryptedFallback();
void setUnencryptedFallback(bool unencryptedFallback);
QString serviceKey();
std::unique_ptr<QSettings> unencryptedSettings();

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

    if (QKeychainFuture::unencryptedFallback()) {
        auto settings = QKeychainFuture::unencryptedSettings();

        settings->beginGroup(service);
        if (settings->contains(key)) {
            promise.addResult(settings->value(key).value<T>());
        } else {
            promise.setException(Error(QKeychain::EntryNotFound, QStringLiteral("Could not find entry")));
            promise.addResult(QKeychain::EntryNotFound);
        }
        settings->endGroup();

        promise.finish();

        return future;
    }

    QMetaObject::invokeMethod(qApp, [service, key, promise = std::move(promise)]() mutable {
        auto job = new QKeychain::ReadPasswordJob(service);

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
    });

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
    return readServiceKey<T>(QKeychainFuture::serviceKey(), key);
}

// Write

template<typename T>
WriteFuture writeServiceKey(const QString &service, const QString &key, const T &value)
{
    QPromise<QFutureValueType<WriteFuture>> promise;
    auto future = promise.future();

    if (QKeychainFuture::unencryptedFallback()) {
        auto settings = QKeychainFuture::unencryptedSettings();

        settings->beginGroup(service);
        settings->setValue(key, value);
        settings->endGroup();
        settings->sync();

        if (settings->status() == QSettings::NoError) {
            promise.addResult(QKeychain::NoError);
        } else {
            promise.setException(Error(QKeychain::OtherError, QStringLiteral("Could not write entry")));
            promise.addResult(QKeychain::OtherError);
        }

        promise.finish();

        return future;
    }

    QMetaObject::invokeMethod(qApp, [service, key, value, promise = std::move(promise)]() mutable {
        auto job = new QKeychain::WritePasswordJob(service);

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
    });

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
    return writeServiceKey(QKeychainFuture::serviceKey(), key, value);
}

// Delete

DeleteFuture deleteServiceKey(const QString &service, const QString &key);
DeleteFuture deleteService(const QString &service);
DeleteFuture deleteKey(const QString &key);
} // QKeychainFuture

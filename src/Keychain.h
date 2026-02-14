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

using namespace std::chrono;

namespace QKeychainFuture
{
using ReadResult = std::variant<QString, QKeychain::Error>;
using WriteResult = QKeychain::Error;
using DeleteResult = WriteResult;

using ReadFuture = QFuture<ReadResult>;
using WriteFuture = QFuture<WriteResult>;
using DeleteFuture = QFuture<DeleteResult>;

bool unencryptedFallback();
void setUnencryptedFallback(bool unencryptedFallback);
QString serviceKey();
QFuture<QKeychain::Error> migrateServiceToEncryptedKeychain(const QString &service);

template<typename T>
bool waitForFinished(const QFuture<T> &future, std::chrono::milliseconds msecs = -1ms)
{
    QDeadlineTimer deadline(msecs);

    do {
        QCoreApplication::processEvents();
    } while (!future.isFinished() && !deadline.hasExpired());

    return future.isFinished();
}

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

ReadFuture readServiceKey(const QString &service, const QString &key);
ReadFuture readService(const QString &service);
ReadFuture readKey(const QString &key);

WriteFuture writeServiceKey(const QString &service, const QString &key, const QString &value);
WriteFuture writeService(const QString &service, const QString &value);
WriteFuture writeKey(const QString &key, const QString &value);

DeleteFuture deleteServiceKey(const QString &service, const QString &key);
DeleteFuture deleteService(const QString &service);
DeleteFuture deleteKey(const QString &key);
} // QKeychainFuture

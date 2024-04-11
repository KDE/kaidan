// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LGPL-2.1-or-later

#pragma once

// std
#include <memory>
// Qt
#include <QFuture>
#include <QtTest>
// QXmpp
#include <QXmppTask.h>

template<typename T>
T wait(const QFuture<T> &future)
{
    auto watcher = std::make_unique<QFutureWatcher<T>>();
    QSignalSpy spy(watcher.get(), &QFutureWatcherBase::finished);
    watcher->setFuture(future);
    spy.wait();
    if constexpr (!std::is_same_v<T, void>) {
        return future.result();
    }
}

template<typename T>
T wait(QObject *context, QXmppTask<T> task)
{
    return wait(task.toFuture(context));
}

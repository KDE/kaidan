// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt
#include <QSignalSpy>
#include <QTest>
// Kaidan
#include "MainController.h"
#include "RosterItemWatcher.h"
#include "Test.h"

using namespace std;

template<typename T, typename Converter>
auto transform(vector<T> &input, Converter convert)
{
    using Output = decay_t<decltype(convert(input.front()))>;
    vector<Output> output;
    output.reserve(input.size());
    transform(input.begin(), input.end(), back_inserter(output), std::move(convert));
    return output;
}

class RosterItemWatcherTest : public Test
{
    Q_OBJECT

private:
    Q_SLOT void notifications();
};

struct SpyCountPair {
    std::unique_ptr<QSignalSpy> spy;
    int count;
};

void checkEmitted(vector<pair<RosterItemWatcher *, int>> watchers, function<void()> trigger)
{
    auto spies = transform(watchers, [](pair<RosterItemWatcher *, int> &input) {
        return SpyCountPair{std::make_unique<QSignalSpy>(input.first, &RosterItemWatcher::itemChanged), input.second};
    });
    trigger();
    for_each(spies.begin(), spies.end(), [](auto &pair) {
        QCOMPARE(pair.spy->size(), pair.count);
    });
}

void RosterItemWatcherTest::notifications()
{
    // This test requires a RosterModel instance, so we init kaidan
    MainController mainController(this);
    RosterItem item;

    const auto accountJid = QStringLiteral("alice@example.org");

    auto &notifier = RosterItemNotifier::instance();
    RosterItemWatcher watcher1;
    watcher1.setAccountJid(accountJid);
    watcher1.setJid(QStringLiteral("hello@kaidan.im"));
    RosterItemWatcher watcher2;
    watcher2.setAccountJid(accountJid);
    watcher2.setJid(QStringLiteral("hello@kaidan.im"));
    RosterItemWatcher watcher3;
    watcher3.setAccountJid(accountJid);
    watcher3.setJid(QStringLiteral("user@kaidan.im"));

    vector<pair<RosterItemWatcher *, int>> expected({{pair{&watcher1, 2}}, {pair{&watcher2, 2}}, {pair{&watcher3, 1}}});
    checkEmitted(expected, [&]() {
        notifier.notifyWatchers(accountJid, QStringLiteral("hello@kaidan.im"), item);
        notifier.notifyWatchers(accountJid, QStringLiteral("not-found"), item);
        notifier.notifyWatchers(accountJid, QStringLiteral("user@kaidan.im"), item);
        notifier.notifyWatchers(accountJid, QStringLiteral("hello@kaidan.im"), item);
        notifier.notifyWatchers(accountJid, QStringLiteral("not-found"), item);
    });
}

QTEST_GUILESS_MAIN(RosterItemWatcherTest)
#include "RosterItemWatcherTest.moc"

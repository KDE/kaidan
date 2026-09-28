// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTest>
// Kaidan
#include "Database.h"
#include "Globals.h"
#include "MediaUtils.h"
#include "Message.h"
#include "MessageDb.h"
#include "SystemUtils.h"
#include "Test.h"
#include "TestUtils.h"

static const QString s_accountJid = QStringLiteral("alice@example.org");
static const QString s_chatJid = QStringLiteral("bob@example.org");

static Message createMessage(const QString &chatJid, const QString &id)
{
    Message message;

    message.accountJid = s_accountJid;
    message.chatJid = chatJid;
    message.isOwn = false;
    message.id = id;
    message.timestamp = QDateTime::currentDateTimeUtc();
    message.setPreparedBody(QStringLiteral("test"));

    return message;
}

class MessageDbTest : public Test
{
    Q_OBJECT

public:
    MessageDbTest();

private:
    Q_SLOT void testRemoveMessage();
    Q_SLOT void testRemoveMessageByOtherId();
    Q_SLOT void testRemoveMessageReactions();
    Q_SLOT void testRemoveMessageFiles();
    Q_SLOT void testMessageModifiableUntilReferenceTime();

    void addMessage(const Message &message);
    bool messageExists(const QString &chatJid, const QString &messageId);

    Database db;
    MessageDb *messageDb = nullptr;
};

MessageDbTest::MessageDbTest()
{
    messageDb = new MessageDb(this);
}

void MessageDbTest::addMessage(const Message &message)
{
    wait(messageDb->addMessage(message, MessageOrigin::Stream));
}

// Removed messages are not part of the "chatMessages" view anymore.
bool MessageDbTest::messageExists(const QString &chatJid, const QString &messageId)
{
    return wait(messageDb->fetchMessage(s_accountJid, chatJid, messageId)).has_value();
}

void MessageDbTest::testRemoveMessage()
{
    const auto messageId = QStringLiteral("remove-1");
    addMessage(createMessage(s_chatJid, messageId));
    QVERIFY(messageExists(s_chatJid, messageId));

    QSignalSpy spy(messageDb, &MessageDb::messageRemoved);
    wait(messageDb->removeMessage(s_accountJid, s_chatJid, messageId));

    QVERIFY(!messageExists(s_chatJid, messageId));

    // The removed message is reported to be able to update the models.
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.constFirst().at(0).value<Message>().id, messageId);
}

// A message is only removed by its own ID because the caller knows the stored message.
void MessageDbTest::testRemoveMessageByOtherId()
{
    const auto messageId = QStringLiteral("remove-2");
    auto message = createMessage(s_chatJid, messageId);
    message.originId = QStringLiteral("remove-2-origin");
    message.stanzaId = QStringLiteral("remove-2-stanza");
    message.replaceId = QStringLiteral("remove-2-replace");
    addMessage(message);

    wait(messageDb->removeMessage(s_accountJid, s_chatJid, message.originId));
    QVERIFY(messageExists(s_chatJid, messageId));

    wait(messageDb->removeMessage(s_accountJid, s_chatJid, message.stanzaId));
    QVERIFY(messageExists(s_chatJid, messageId));

    wait(messageDb->removeMessage(s_accountJid, s_chatJid, message.replaceId));
    QVERIFY(messageExists(s_chatJid, messageId));

    wait(messageDb->removeMessage(s_accountJid, s_chatJid, messageId));
    QVERIFY(!messageExists(s_chatJid, messageId));
}

// Reactions reference a message by the ID other clients know it by instead of its own ID.
void MessageDbTest::testRemoveMessageReactions()
{
    auto message = createMessage(s_chatJid, QStringLiteral("remove-reactions"));
    message.originId = QStringLiteral("remove-reactions-origin");
    addMessage(message);

    wait(messageDb->updateMessage(s_accountJid, s_chatJid, message.id, [](Message &message) {
        MessageReaction reaction;
        reaction.emoji = QStringLiteral("👍");
        reaction.deliveryState = MessageReactionDeliveryState::PendingAddition;

        auto &reactionSender = message.reactionSenders[s_accountJid];
        reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();
        reactionSender.reactions.append(reaction);
    }));
    QVERIFY(wait(messageDb->fetchPendingReactions(s_accountJid)).value(s_chatJid).contains(message.originId));

    wait(messageDb->removeMessage(s_accountJid, s_chatJid, message.id));
    QVERIFY(!wait(messageDb->fetchPendingReactions(s_accountJid)).value(s_chatJid).contains(message.originId));
}

// Downloaded files are deleted as well because a message can be removed while it is not
// displayed.
void MessageDbTest::testRemoveMessageFiles()
{
    const auto downloadDirectory = SystemUtils::downloadDirectory();
    QVERIFY(QDir().mkpath(downloadDirectory));

    const auto localFilePath = downloadDirectory + QDir::separator() + QStringLiteral("remove-files.txt");
    QFile localFile(localFilePath);
    QVERIFY(localFile.open(QIODevice::WriteOnly));
    localFile.close();

    File file;
    file.id = 4242;
    file.fileGroupId = 4242;
    file.mimeType = MediaUtils::mimeDatabase().mimeTypeForFile(localFilePath);
    file.localFilePath = localFilePath;

    auto message = createMessage(s_chatJid, QStringLiteral("remove-files"));
    message.fileGroupId = file.fileGroupId;
    message.files = {file};
    addMessage(message);

    wait(messageDb->removeMessage(s_accountJid, s_chatJid, message.id));
    QVERIFY(!QFile::exists(localFilePath));
}

// Only the messages until the modification count, e.g., if the modification is received from an
// archive after more recent messages.
void MessageDbTest::testMessageModifiableUntilReferenceTime()
{
    const auto chatJid = QStringLiteral("erin@example.org");
    const auto timestamp = QDateTime::currentDateTimeUtc().addSecs(-3600);

    auto message = createMessage(chatJid, QStringLiteral("modify-until-reference-time"));
    message.timestamp = timestamp;
    addMessage(message);

    for (int i = 1; i <= MAX_MESSAGE_MODIFICATION_COUNT; i++) {
        auto moreRecentMessage = createMessage(chatJid, QStringLiteral("modify-until-reference-time-%1").arg(i));
        moreRecentMessage.timestamp = timestamp.addSecs(60 + i);
        addMessage(moreRecentMessage);
    }

    // The more recent messages have been exchanged after the modification.
    QVERIFY(wait(messageDb->isMessageModifiable(message, false, timestamp.addSecs(60))));

    // Too many more recent messages have been exchanged before the modification.
    QVERIFY(!wait(messageDb->isMessageModifiable(message, false, QDateTime::currentDateTimeUtc())));
}

QTEST_GUILESS_MAIN(MessageDbTest)
#include "MessageDbTest.moc"

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
#include "GroupChatUserDb.h"
#include "MediaUtils.h"
#include "Message.h"
#include "MessageDb.h"
#include "SystemUtils.h"
#include "Test.h"
#include "TestUtils.h"

static const QString s_accountJid = QStringLiteral("alice@example.org");
static const QString s_chatJid = QStringLiteral("bob@example.org");
static const QString s_otherChatJid = QStringLiteral("carol@example.org");
// Used for tests that depend on the messages of the whole chat.
static const QString s_modificationChatJid = QStringLiteral("dave@example.org");

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

static Message createGroupChatMessage(const QString &chatJid, const QString &id, const QString &senderId)
{
    auto message = createMessage(chatJid, id);

    message.groupChatSenderId = senderId;

    return message;
}

// Accepts each retraction in order to test the message lookup itself.
static bool acceptRetraction(const Message &)
{
    return true;
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
    Q_SLOT void testApplyMessageRetraction();
    Q_SLOT void testApplyMessageRetractionByStanzaId();
    Q_SLOT void testApplyGroupChatMessageRetraction();
    Q_SLOT void testApplyMessageRetractionWithoutId();
    Q_SLOT void testApplyMessageRetractionOfOtherChat();
    Q_SLOT void testApplyMessageRetractionOfOtherAccount();
    Q_SLOT void testApplyInvalidMessageRetraction();
    Q_SLOT void testApplyMessageRetractionWithAmbiguousId();
    Q_SLOT void testMessageModifiable();
    Q_SLOT void testPendingMessageRetraction();
    Q_SLOT void testMessageRetractionError();
    Q_SLOT void testFetchPendingMessageRetractionsOfOtherAccount();
    Q_SLOT void testUpdateAndRemoveExactMessage();

    void addMessage(const Message &message);
    bool messageExists(const QString &chatJid, const QString &messageId);
    void setRetractionState(const Message &message, Message::RetractionState state, const QString &errorText = {});
    QList<QString> pendingMessageRetractionIds(const QString &accountJid = s_accountJid);

    Database db;
    MessageDb *messageDb = nullptr;
};

MessageDbTest::MessageDbTest()
{
    // Needed because the sender of a group chat message is looked up while storing it.
    new GroupChatUserDb(this);

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

void MessageDbTest::setRetractionState(const Message &message, Message::RetractionState state, const QString &errorText)
{
    wait(messageDb->updateMessage(message.accountJid, message.chatJid, message.id, [state, errorText](Message &message) {
        message.retractionState = state;
        message.errorText = errorText;
    }));
}

QList<QString> MessageDbTest::pendingMessageRetractionIds(const QString &accountJid)
{
    QList<QString> ids;

    for (const auto &message : wait(messageDb->fetchPendingMessageRetractions(accountJid))) {
        ids.append(message.id);
    }

    return ids;
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

// A retraction references a message by the ID of the stanza that has been sent or received.
void MessageDbTest::testApplyMessageRetraction()
{
    const auto messageId = QStringLiteral("retract-id");
    addMessage(createMessage(s_chatJid, messageId));

    wait(messageDb->applyMessageRetraction(s_accountJid, s_chatJid, messageId, acceptRetraction));
    QVERIFY(!messageExists(s_chatJid, messageId));

    const auto originMessageId = QStringLiteral("retract-origin");
    auto originMessage = createMessage(s_chatJid, originMessageId);
    originMessage.originId = QStringLiteral("retract-origin-id");
    addMessage(originMessage);

    wait(messageDb->applyMessageRetraction(s_accountJid, s_chatJid, originMessage.originId, acceptRetraction));
    QVERIFY(!messageExists(s_chatJid, originMessageId));

    // A corrected message is still referenced by the ID of the message it corrected.
    const auto correctedMessageId = QStringLiteral("retract-corrected");
    auto correctedMessage = createMessage(s_chatJid, correctedMessageId);
    correctedMessage.replaceId = QStringLiteral("retract-replaced-id");
    addMessage(correctedMessage);

    wait(messageDb->applyMessageRetraction(s_accountJid, s_chatJid, correctedMessage.replaceId, acceptRetraction));
    QVERIFY(!messageExists(s_chatJid, correctedMessageId));
}

// The stanza ID of a direct chat message is assigned by the own server and thus never used to
// reference a retracted message.
void MessageDbTest::testApplyMessageRetractionByStanzaId()
{
    const auto messageId = QStringLiteral("retract-stanza");
    auto message = createMessage(s_chatJid, messageId);
    message.stanzaId = QStringLiteral("retract-stanza-id");
    addMessage(message);

    wait(messageDb->applyMessageRetraction(s_accountJid, s_chatJid, message.stanzaId, acceptRetraction));
    QVERIFY(messageExists(s_chatJid, messageId));
}

// A group chat message is referenced by the stanza ID assigned by the group chat.
void MessageDbTest::testApplyGroupChatMessageRetraction()
{
    const auto messageId = QStringLiteral("retract-group-chat");
    auto message = createGroupChatMessage(s_chatJid, messageId, QStringLiteral("participant-1"));
    message.stanzaId = QStringLiteral("retract-group-chat-stanza-id");
    addMessage(message);

    wait(messageDb->applyMessageRetraction(s_accountJid, s_chatJid, message.stanzaId, acceptRetraction));
    QVERIFY(!messageExists(s_chatJid, messageId));

    // A corrected message is still referenced by the ID of the message it corrected.
    const auto correctedMessageId = QStringLiteral("retract-group-chat-corrected");
    auto correctedMessage = createGroupChatMessage(s_chatJid, correctedMessageId, QStringLiteral("participant-1"));
    correctedMessage.stanzaId = QStringLiteral("retract-group-chat-corrected-stanza-id");
    correctedMessage.replaceId = QStringLiteral("retract-group-chat-replaced-id");
    addMessage(correctedMessage);

    wait(messageDb->applyMessageRetraction(s_accountJid, s_chatJid, correctedMessage.replaceId, acceptRetraction));
    QVERIFY(!messageExists(s_chatJid, correctedMessageId));
}

// An empty ID must not match messages without an origin or replace ID.
void MessageDbTest::testApplyMessageRetractionWithoutId()
{
    const auto messageId = QStringLiteral("retract-empty");
    addMessage(createMessage(s_chatJid, messageId));

    bool checked = false;
    wait(messageDb->applyMessageRetraction(s_accountJid, s_chatJid, {}, [&checked](const Message &) {
        checked = true;
        return true;
    }));

    QVERIFY(!checked);
    QVERIFY(messageExists(s_chatJid, messageId));
}

// A message must not be retractable from another chat, even if its IDs are known.
void MessageDbTest::testApplyMessageRetractionOfOtherChat()
{
    const auto messageId = QStringLiteral("retract-other-chat");
    addMessage(createMessage(s_chatJid, messageId));

    wait(messageDb->applyMessageRetraction(s_accountJid, s_otherChatJid, messageId, acceptRetraction));
    QVERIFY(messageExists(s_chatJid, messageId));
}

void MessageDbTest::testApplyMessageRetractionOfOtherAccount()
{
    const auto messageId = QStringLiteral("retract-other-account");
    addMessage(createMessage(s_chatJid, messageId));

    wait(messageDb->applyMessageRetraction(QStringLiteral("eve@example.org"), s_chatJid, messageId, acceptRetraction));
    QVERIFY(messageExists(s_chatJid, messageId));
}

void MessageDbTest::testApplyInvalidMessageRetraction()
{
    const auto messageId = QStringLiteral("retract-invalid");
    addMessage(createMessage(s_chatJid, messageId));

    QSignalSpy spy(messageDb, &MessageDb::messageRemoved);
    wait(messageDb->applyMessageRetraction(s_accountJid, s_chatJid, messageId, [](const Message &) {
        return false;
    }));

    QVERIFY(messageExists(s_chatJid, messageId));
    QCOMPARE(spy.count(), 0);
}

// Message IDs are only unique if the sending client follows XEP-0424.
void MessageDbTest::testApplyMessageRetractionWithAmbiguousId()
{
    const auto messageId = QStringLiteral("retract-ambiguous");
    const auto timestamp = QDateTime::currentDateTimeUtc();

    auto olderMessage = createMessage(s_chatJid, messageId);
    olderMessage.originId = QStringLiteral("retract-ambiguous-older");
    olderMessage.timestamp = timestamp.addSecs(-60);
    addMessage(olderMessage);

    auto newerMessage = createMessage(s_chatJid, messageId);
    newerMessage.originId = QStringLiteral("retract-ambiguous-newer");
    newerMessage.timestamp = timestamp;
    addMessage(newerMessage);

    wait(messageDb->applyMessageRetraction(s_accountJid, s_chatJid, messageId, acceptRetraction));

    // The most recent matching message is removed to be deterministic.
    QVERIFY(!messageExists(s_chatJid, newerMessage.originId));
    QVERIFY(messageExists(s_chatJid, olderMessage.originId));
}

// The same limits are applied to corrections and retractions.
void MessageDbTest::testMessageModifiable()
{
    const auto modifiedByContact = [this](const Message &message) {
        return messageDb->_isMessageModifiable(message, false, QDateTime::currentDateTimeUtc());
    };

    // Only the author of a message may modify it.
    const auto ownMessageId = QStringLiteral("modify-own");
    auto ownMessage = createMessage(s_modificationChatJid, ownMessageId);
    ownMessage.isOwn = true;
    ownMessage.timestamp = QDateTime::currentDateTimeUtc().addSecs(-7200);
    addMessage(ownMessage);

    wait(messageDb->applyMessageRetraction(s_accountJid, s_modificationChatJid, ownMessageId, modifiedByContact));
    QVERIFY(messageExists(s_modificationChatJid, ownMessageId));

    // A message must not be too old.
    const auto oldMessageId = QStringLiteral("modify-old");
    auto oldMessage = createMessage(s_modificationChatJid, oldMessageId);
    oldMessage.timestamp = QDateTime::currentDateTimeUtc().addDays(-MAX_MESSAGE_MODIFICATION_DAYS).addSecs(-1);
    addMessage(oldMessage);

    wait(messageDb->applyMessageRetraction(s_accountJid, s_modificationChatJid, oldMessageId, modifiedByContact));
    QVERIFY(messageExists(s_modificationChatJid, oldMessageId));

    // A message may be modified if it is among the most recent messages.
    // The messages are in the past because only the messages until the modification are counted.
    const auto timestamp = QDateTime::currentDateTimeUtc().addSecs(-3600);
    const auto recentMessageId = QStringLiteral("modify-recent");
    auto recentMessage = createMessage(s_modificationChatJid, recentMessageId);
    recentMessage.timestamp = timestamp;
    addMessage(recentMessage);

    for (int i = 1; i < MAX_MESSAGE_MODIFICATION_COUNT; i++) {
        auto moreRecentMessage = createMessage(s_modificationChatJid, QStringLiteral("modify-more-recent-%1").arg(i));
        moreRecentMessage.timestamp = timestamp.addSecs(i);
        addMessage(moreRecentMessage);
    }

    wait(messageDb->applyMessageRetraction(s_accountJid, s_modificationChatJid, recentMessageId, modifiedByContact));
    QVERIFY(!messageExists(s_modificationChatJid, recentMessageId));

    // There must not be too many more recent messages.
    const auto oldestMessageId = QStringLiteral("modify-oldest");
    auto oldestMessage = createMessage(s_modificationChatJid, oldestMessageId);
    oldestMessage.timestamp = timestamp;
    addMessage(oldestMessage);

    auto newestMessage = createMessage(s_modificationChatJid, QStringLiteral("modify-newest"));
    newestMessage.timestamp = timestamp.addSecs(MAX_MESSAGE_MODIFICATION_COUNT);
    addMessage(newestMessage);

    wait(messageDb->applyMessageRetraction(s_accountJid, s_modificationChatJid, oldestMessageId, modifiedByContact));
    QVERIFY(messageExists(s_modificationChatJid, oldestMessageId));
}

// A message whose retraction is pending is fetched to send the retraction once there is a
// connection.
void MessageDbTest::testPendingMessageRetraction()
{
    auto message = createMessage(s_chatJid, QStringLiteral("retraction-pending"));
    message.isOwn = true;
    addMessage(message);
    QVERIFY(!pendingMessageRetractionIds().contains(message.id));

    setRetractionState(message, Message::RetractionState::Pending);
    QVERIFY(pendingMessageRetractionIds().contains(message.id));

    // Once the retraction has been sent, the message is removed and not fetched anymore.
    wait(messageDb->removeMessage(s_accountJid, s_chatJid, message.id));
    QVERIFY(!pendingMessageRetractionIds().contains(message.id));
}

// A retraction that could not be sent is only sent again when the user retries it.
void MessageDbTest::testMessageRetractionError()
{
    const auto errorText = QStringLiteral("service unavailable");

    auto message = createMessage(s_chatJid, QStringLiteral("retraction-error"));
    message.isOwn = true;
    addMessage(message);

    setRetractionState(message, Message::RetractionState::Error, errorText);
    QVERIFY(!pendingMessageRetractionIds().contains(message.id));

    const auto storedMessage = wait(messageDb->fetchMessage(s_accountJid, s_chatJid, message.id));
    QVERIFY(storedMessage);
    QCOMPARE(storedMessage->retractionState, Message::RetractionState::Error);
    QCOMPARE(storedMessage->errorText, errorText);

    setRetractionState(message, Message::RetractionState::Pending);
    QVERIFY(pendingMessageRetractionIds().contains(message.id));

    wait(messageDb->removeMessage(s_accountJid, s_chatJid, message.id));
}

void MessageDbTest::testFetchPendingMessageRetractionsOfOtherAccount()
{
    auto message = createMessage(s_chatJid, QStringLiteral("retraction-other-account"));
    message.accountJid = QStringLiteral("eve@example.org");
    message.isOwn = true;
    addMessage(message);

    setRetractionState(message, Message::RetractionState::Pending);
    QVERIFY(!pendingMessageRetractionIds().contains(message.id));
    QVERIFY(pendingMessageRetractionIds(message.accountJid).contains(message.id));

    wait(messageDb->removeMessages(message.accountJid));
}

// A message whose retraction has been sent is updated or removed by its ID and timestamp instead of
// a message that references the same ID.
void MessageDbTest::testUpdateAndRemoveExactMessage()
{
    const auto timestamp = QDateTime::currentDateTimeUtc();
    const auto errorText = QStringLiteral("service unavailable");

    auto message = createMessage(s_chatJid, QStringLiteral("exact"));
    message.isOwn = true;
    message.originId = QStringLiteral("exact-origin");
    message.timestamp = timestamp.addSecs(-60);
    addMessage(message);

    auto otherMessage = createMessage(s_chatJid, QStringLiteral("exact-other"));
    otherMessage.replaceId = message.originId;
    otherMessage.timestamp = timestamp;
    addMessage(otherMessage);

    wait(messageDb->updateMessage(message, [errorText](Message &storedMessage) {
        storedMessage.errorText = errorText;
    }));
    QCOMPARE(wait(messageDb->fetchMessage(s_accountJid, s_chatJid, message.id))->errorText, errorText);
    QVERIFY(wait(messageDb->fetchMessage(s_accountJid, s_chatJid, otherMessage.id))->errorText.isEmpty());

    wait(messageDb->removeMessage(message));
    QVERIFY(!messageExists(s_chatJid, message.id));
    QVERIFY(messageExists(s_chatJid, otherMessage.id));
}

QTEST_GUILESS_MAIN(MessageDbTest)
#include "MessageDbTest.moc"

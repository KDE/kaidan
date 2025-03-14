// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Qt
#include <QTest>
// Kaidan
#include "Database.h"
#include "OmemoDb.h"
#include "Test.h"
#include "TestUtils.h"

using Storage = OmemoDb;

bool operator==(const Storage::OwnDevice &a, const Storage::OwnDevice &b)
{
    return a.id == b.id && a.label == b.label && a.latestPreKeyId == b.latestPreKeyId && a.latestSignedPreKeyId == b.latestSignedPreKeyId
        && a.privateIdentityKey == b.privateIdentityKey && a.publicIdentityKey == b.publicIdentityKey;
}
bool operator==(const Storage::Device &a, const Storage::Device &b)
{
    return a.label == b.label && a.keyId == b.keyId && a.session == b.session && a.unrespondedReceivedStanzasCount == b.unrespondedReceivedStanzasCount
        && a.unrespondedSentStanzasCount == b.unrespondedSentStanzasCount && a.removalFromDeviceListDate == b.removalFromDeviceListDate;
}
bool operator==(const Storage::SignedPreKeyPair &a, const Storage::SignedPreKeyPair &b)
{
    return a.data == b.data && a.creationDate == b.creationDate;
}

class OmemoDbTest : public Test
{
    Q_OBJECT

private:
    Q_SLOT void testOwnDevice();
    Q_SLOT void testSignedPreKeyPairs();
    Q_SLOT void testPreKeyPairs();
    Q_SLOT void testDevices();
    Q_SLOT void testResetAll();

    Database db;
    OmemoDb storage = OmemoDb(&db, this, QStringLiteral("user@example.org"), this);
};

void OmemoDbTest::testOwnDevice()
{
    QVERIFY(!wait(this, storage.allData()).ownDevice.has_value());

    Storage::OwnDevice ownDevice;
    storage.setOwnDevice(ownDevice);

    // Check the default values.
    auto data = wait(this, storage.allData());
    QVERIFY(data.ownDevice.has_value());
    QCOMPARE(data.ownDevice.value(), ownDevice);

    ownDevice = Storage::OwnDevice{
        .id = 1,
        .label = QStringLiteral("Notebook"),
        .privateIdentityKey = QByteArray::fromBase64("ZDVNZFdJeFFUa3N6ZWdSUG9scUdoQXFpWERGbHRsZTIK"),
        .publicIdentityKey = QByteArray::fromBase64("dUsxSTJyM2tKVHE1TzNXbk1Xd0tpMGY0TnFleDRYUGkK"),
        .latestSignedPreKeyId = 2,
        .latestPreKeyId = 100,
    };
    storage.setOwnDevice(ownDevice);

    // Check the set values.
    QCOMPARE(wait(this, storage.allData()).ownDevice, ownDevice);
}

void OmemoDbTest::testSignedPreKeyPairs()
{
    Storage::SignedPreKeyPairs signedPreKeyPairs;
    QCOMPARE(wait(this, storage.allData()).signedPreKeyPairs, signedPreKeyPairs);

    // add values
    auto pair1 = Storage::SignedPreKeyPair{
        .creationDate = QDateTime(QDate(2022, 01, 01), QTime()),
        .data = "FaZmWjwqppAoMff72qTzUIktGUbi4pAmds1Cuh6OElmi",
    };
    auto pair2 = Storage::SignedPreKeyPair{
        .creationDate = QDateTime(QDate(2022, 01, 02), QTime()),
        .data = "jsrj4UYQqaHJrlysNu0uoHgmAU8ffknPpwKJhdqLYgIU",
    };
    signedPreKeyPairs = {{1, pair1}, {2, pair2}};

    // check values have been added
    storage.addSignedPreKeyPair(1, pair1);
    storage.addSignedPreKeyPair(2, pair2);
    QCOMPARE(wait(this, storage.allData()).signedPreKeyPairs, signedPreKeyPairs);

    // check values have been removed
    signedPreKeyPairs.remove(1);
    storage.removeSignedPreKeyPair(1);
    QCOMPARE(wait(this, storage.allData()).signedPreKeyPairs, signedPreKeyPairs);
}

void OmemoDbTest::testPreKeyPairs()
{
    // empty check
    auto data = wait(this, storage.allData());
    QCOMPARE(data.preKeyPairs.size(), 0);
    QCOMPARE(data.preKeyPairs, Storage::PreKeyPairs());

    const std::array<std::pair<uint32_t, QByteArray>, 3> pairs = {
        std::pair{1, "RZLgD0lmL2WpJbskbGKFRMZL4zqSSvU0rElmO7UwGSVt"},
        std::pair{2, "3PGPNsf9P7pPitp9dt2uvZYT4HkxdHJAbWqLvOPXUeca"},
        std::pair{3, "LpLBVXejfU4d0qcPOJCRNDDg9IMbOujpV3UTYtZU9LTy"},
    };
    Storage::PreKeyPairs preKeyPairs(pairs.begin(), pairs.end());

    storage.addPreKeyPairs({pairs[0], pairs[1]});
    storage.addPreKeyPairs({pairs[2]});
    QCOMPARE(wait(this, storage.allData()).preKeyPairs, preKeyPairs);

    preKeyPairs.remove(1);
    storage.removePreKeyPair(1);
    QCOMPARE(wait(this, storage.allData()).preKeyPairs, preKeyPairs);
}

void OmemoDbTest::testDevices()
{
    // empty check
    QCOMPARE(wait(this, storage.allData()).devices, Storage::Devices());

    Storage::Device deviceAlice = {.label = QStringLiteral("Desktop"),
                                   .keyId = QByteArray::fromBase64("bEFLaDRQRkFlYXdyakE2aURoN0wyMzk2NTJEM2hRMgo="),
                                   .session = QByteArray::fromBase64("Cs8CCAQSIQWIhBRMdJ80tLVT7ius0H1LutRLeXBid68NH90M/kwhGxohBT+2kM/wV"
                                                                     "Q2UrZZPJBRmGZP0ZoCCWiET7KxA3ieAa888IiBSTWnp4qrTeo7z9kfKRaAFy+fYwP"
                                                                     "BI2HCSOxfC0anyPigAMmsKIQXZ95Xs7I+tOsg76eLtp266XTuCF8STa+VZkXPPJ00"
                                                                     "WSRIgmJ73wjhXPZqIt9ofB0NVwbWOKnYzQ90SHJEd/hyBHkUaJAgAEiDxXDT00+zp"
                                                                     "Jd+TKJrD6nWQxQZhB8I7vCRdD/Oxw61MYjpJCiEFmTV1l+cOLEytoTp17VOEunYlC"
                                                                     "ZmDqn/qoUYI/8P9ZQsaJAgBEiB/QP+9Lb0YOhSQmIr/X75Vs1FME1qzmohSzqBVTz"
                                                                     "bfZFCnf1jsR2AAaiEFPxj3VK+knGrndOjcgMXI4wEfH/0VrbgJqobGWbewYyA="),
                                   .unrespondedSentStanzasCount = 10,
                                   .unrespondedReceivedStanzasCount = 11,
                                   .removalFromDeviceListDate = QDateTime(QDate(2022, 01, 01), QTime())};

    Storage::Device deviceBob1;
    deviceBob1.label = QStringLiteral("Phone");
    deviceBob1.keyId = QByteArray::fromBase64(("WTV6c3B2UFhYbE9OQ1d0N0ZScUhLWXpmYnY2emJoego="));
    deviceBob1.session =
        QByteArray::fromBase64(("CvgCCAQSIQXZwE+G9R6ECMxKWPMidwcx3lPboUT2KEoea3B2T3vjUBohBQ7qW+Fb9Gi/"
                                "SLsuQTv2TRixF0zLx2/mw0V4arjYSmgHIiCwuvEP2eyFU7FsbtSZBWKt+hH/DwBF7C0W"
                                "rfxDrSu1bSgAMmsKIQXm5tRa73ZcUWn7fQa2YlDv+yLw1copPjdRZCrGcK7cNRIg0OXB"
                                "vqBTAfyiUlLKW3LDIiSMHkRYYWDyknSJz3s+81oaJAgAEiAQlSKV+70EMYAjjW88dO52"
                                "dp9e/aDhT8YUDHNFaCFUxTpJCiEF2OE4fb7Quwg0PMeJfT1uXmq/YXVaos9A7bn37TyS"
                                "iWkaJAgAEiDJlr5w0mBHBHZzttfVyvd2y2IzBV7bGdoX+lKHaEGIoUonCAwSIQXN7Y76"
                                "Vwcsaubw8EHYaIPnBB11WjEEYcEPalwlgEUECRgCUMgnWMgnYABqIQXN7Y76Vwcsaubw"
                                "8EHYaIPnBB11WjEEYcEPalwlgEUECQ=="));
    deviceBob1.unrespondedSentStanzasCount = 20;
    deviceBob1.unrespondedReceivedStanzasCount = 21;
    deviceBob1.removalFromDeviceListDate = QDateTime(QDate(2022, 01, 02), QTime());

    Storage::Device deviceBob2;
    deviceBob2.label = QStringLiteral("Tablet");
    deviceBob2.keyId = QByteArray::fromBase64(("U0tXcUlSVHVISzZLYUdGcW53czBtdXYxTEt2blVsbQo="));
    deviceBob2.session =
        QByteArray::fromBase64(("CvgCCAQSIQU/tpDP8FUNlK2WTyQUZhmT9GaAglohE+ysQN4ngGvPPBohBdnAT4b1HoQI"
                                "zEpY8yJ3BzHeU9uhRPYoSh5rcHZPe+NQIiBNmwyjLm5xdbf5f9ab9AASopfdiSybMFMd"
                                "S4SQR5pSTygAMmsKIQW5FhVKpKUzKlhUCfoCmMwoo5jUFn7+NrcOQl6CQYraZRIgkNHG"
                                "SWgeoLUvYMM8wsgqU4RUv8ymv/Kv4LLJb8q4vlEaJAgAEiA/GmWir7/6tWyOTrGXsehU"
                                "nnPZhFs6zGvTDNe1LZaIeTpJCiEFa7t/sVQV2uofS36GbijY63d2B4yJKFGDu6K96cU5"
                                "PFsaJAgAEiA6kX2jqwfZkN0AmNOZGLPg9J8ryrSSpo74DxU85z0q/konCE4SIQWZRzzF"
                                "f3M1/gzbg9/xUsNcyiUnr5jAjLpSPOj7BOW6BBgCUKd/WKd/YABqIQWZRzzFf3M1/gzb"
                                "g9/xUsNcyiUnr5jAjLpSPOj7BOW6BA=="));
    deviceBob2.unrespondedSentStanzasCount = 30;
    deviceBob2.unrespondedReceivedStanzasCount = 31;
    deviceBob2.removalFromDeviceListDate = QDateTime(QDate(2022, 01, 03), QTime());

    storage.addDevice(QStringLiteral("alice@example.org"), 1, deviceAlice);
    storage.addDevice(QStringLiteral("bob@example.com"), 1, deviceBob1);
    storage.addDevice(QStringLiteral("bob@example.com"), 2, deviceBob2);

    {
        auto result = wait(this, storage.allData()).devices;
        QCOMPARE(result.size(), 2);

        auto resultDevicesAlice = result.value(QStringLiteral("alice@example.org"));
        QCOMPARE(resultDevicesAlice.size(), 1);

        auto resultDeviceAlice = resultDevicesAlice.value(1);
        QCOMPARE(resultDeviceAlice.label, QStringLiteral("Desktop"));
        QCOMPARE(resultDeviceAlice.keyId, QByteArray::fromBase64(("bEFLaDRQRkFlYXdyakE2aURoN0wyMzk2NTJEM2hRMgo=")));
        QCOMPARE(resultDeviceAlice.session,
                 QByteArray::fromBase64(("Cs8CCAQSIQWIhBRMdJ80tLVT7ius0H1LutRLeXBid68NH90M/"
                                         "kwhGxohBT+2kM/"
                                         "wVQ2UrZZPJBRmGZP0ZoCCWiET7KxA3ieAa888IiBSTWnp4qrTeo7z9k"
                                         "fKRaAFy+fYwPBI2HCSOxfC0anyPigAMmsKIQXZ95Xs7I+"
                                         "tOsg76eLtp266XTuCF8STa+"
                                         "VZkXPPJ00WSRIgmJ73wjhXPZqIt9ofB0NVwbWOKnYzQ90SHJEd/"
                                         "hyBHkUaJAgAEiDxXDT00+zpJd+TKJrD6nWQxQZhB8I7vCRdD/"
                                         "Oxw61MYjpJCiEFmTV1l+cOLEytoTp17VOEunYlCZmDqn/qoUYI/"
                                         "8P9ZQsaJAgBEiB/QP+9Lb0YOhSQmIr/"
                                         "X75Vs1FME1qzmohSzqBVTzbfZFCnf1jsR2AAaiEFPxj3VK+"
                                         "knGrndOjcgMXI4wEfH/0VrbgJqobGWbewYyA=")));
        QCOMPARE(resultDeviceAlice.unrespondedSentStanzasCount, 10);
        QCOMPARE(resultDeviceAlice.unrespondedReceivedStanzasCount, 11);
        QCOMPARE(resultDeviceAlice.removalFromDeviceListDate, QDateTime(QDate(2022, 01, 01), QTime()));

        auto resultDevicesBob = result.value(QStringLiteral("bob@example.com"));
        QCOMPARE(resultDevicesBob.size(), 2);

        auto resultDeviceBob1 = resultDevicesBob.value(1);
        QCOMPARE(resultDeviceBob1.label, QStringLiteral("Phone"));
        QCOMPARE(resultDeviceBob1.keyId, QByteArray::fromBase64(("WTV6c3B2UFhYbE9OQ1d0N0ZScUhLWXpmYnY2emJoego=")));
        QCOMPARE(resultDeviceBob1.session,
                 QByteArray::fromBase64(("CvgCCAQSIQXZwE+"
                                         "G9R6ECMxKWPMidwcx3lPboUT2KEoea3B2T3vjUBohBQ7qW+Fb9Gi/"
                                         "SLsuQTv2TRixF0zLx2/"
                                         "mw0V4arjYSmgHIiCwuvEP2eyFU7FsbtSZBWKt+hH/"
                                         "DwBF7C0WrfxDrSu1bSgAMmsKIQXm5tRa73ZcUWn7fQa2YlDv+"
                                         "yLw1copPjdRZCrGcK7cNRIg0OXBvqBTAfyiUlLKW3LDIiSMHkRYYWDy"
                                         "knSJz3s+81oaJAgAEiAQlSKV+70EMYAjjW88dO52dp9e/"
                                         "aDhT8YUDHNFaCFUxTpJCiEF2OE4fb7Quwg0PMeJfT1uXmq/"
                                         "YXVaos9A7bn37TySiWkaJAgAEiDJlr5w0mBHBHZzttfVyvd2y2IzBV7"
                                         "bGdoX+"
                                         "lKHaEGIoUonCAwSIQXN7Y76Vwcsaubw8EHYaIPnBB11WjEEYcEPalwl"
                                         "gEUECRgCUMgnWMgnYABqIQXN7Y76Vwcsaubw8EHYaIPnBB11WjEEYcE"
                                         "PalwlgEUECQ==")));
        QCOMPARE(resultDeviceBob1.unrespondedSentStanzasCount, 20);
        QCOMPARE(resultDeviceBob1.unrespondedReceivedStanzasCount, 21);
        QCOMPARE(resultDeviceBob1.removalFromDeviceListDate, QDateTime(QDate(2022, 01, 02), QTime()));

        auto resultDeviceBob2 = resultDevicesBob.value(2);
        QCOMPARE(resultDeviceBob2.label, QStringLiteral("Tablet"));
        QCOMPARE(resultDeviceBob2.keyId, QByteArray::fromBase64(("U0tXcUlSVHVISzZLYUdGcW53czBtdXYxTEt2blVsbQo=")));
        QCOMPARE(resultDeviceBob2.session,
                 QByteArray::fromBase64(("CvgCCAQSIQU/"
                                         "tpDP8FUNlK2WTyQUZhmT9GaAglohE+"
                                         "ysQN4ngGvPPBohBdnAT4b1HoQIzEpY8yJ3BzHeU9uhRPYoSh5rcHZPe+"
                                         "NQIiBNmwyjLm5xdbf5f9ab9AASopfdiSybMFMdS4SQR5pSTygAMmsKIQ"
                                         "W5FhVKpKUzKlhUCfoCmMwoo5jUFn7+"
                                         "NrcOQl6CQYraZRIgkNHGSWgeoLUvYMM8wsgqU4RUv8ymv/"
                                         "Kv4LLJb8q4vlEaJAgAEiA/GmWir7/"
                                         "6tWyOTrGXsehUnnPZhFs6zGvTDNe1LZaIeTpJCiEFa7t/"
                                         "sVQV2uofS36GbijY63d2B4yJKFGDu6K96cU5PFsaJAgAEiA6kX2jqwfZ"
                                         "kN0AmNOZGLPg9J8ryrSSpo74DxU85z0q/konCE4SIQWZRzzFf3M1/"
                                         "gzbg9/xUsNcyiUnr5jAjLpSPOj7BOW6BBgCUKd/WKd/"
                                         "YABqIQWZRzzFf3M1/gzbg9/xUsNcyiUnr5jAjLpSPOj7BOW6BA==")));
        QCOMPARE(resultDeviceBob2.unrespondedSentStanzasCount, 30);
        QCOMPARE(resultDeviceBob2.unrespondedReceivedStanzasCount, 31);
        QCOMPARE(resultDeviceBob2.removalFromDeviceListDate, QDateTime(QDate(2022, 01, 03), QTime()));
    }

    storage.removeDevice(QStringLiteral("bob@example.com"), 2);

    {
        auto result = wait(this, storage.allData()).devices;
        QCOMPARE(result.size(), 2);

        auto resultDevicesAlice = result.value(QStringLiteral("alice@example.org"));
        QCOMPARE(resultDevicesAlice.size(), 1);

        auto resultDeviceAlice = resultDevicesAlice.value(1);
        QCOMPARE(resultDeviceAlice.label, QStringLiteral("Desktop"));
        QCOMPARE(resultDeviceAlice.keyId, QByteArray::fromBase64(("bEFLaDRQRkFlYXdyakE2aURoN0wyMzk2NTJEM2hRMgo=")));
        QCOMPARE(resultDeviceAlice.session,
                 QByteArray::fromBase64(("Cs8CCAQSIQWIhBRMdJ80tLVT7ius0H1LutRLeXBid68NH90M/"
                                         "kwhGxohBT+2kM/"
                                         "wVQ2UrZZPJBRmGZP0ZoCCWiET7KxA3ieAa888IiBSTWnp4qrTeo7z9k"
                                         "fKRaAFy+fYwPBI2HCSOxfC0anyPigAMmsKIQXZ95Xs7I+"
                                         "tOsg76eLtp266XTuCF8STa+"
                                         "VZkXPPJ00WSRIgmJ73wjhXPZqIt9ofB0NVwbWOKnYzQ90SHJEd/"
                                         "hyBHkUaJAgAEiDxXDT00+zpJd+TKJrD6nWQxQZhB8I7vCRdD/"
                                         "Oxw61MYjpJCiEFmTV1l+cOLEytoTp17VOEunYlCZmDqn/qoUYI/"
                                         "8P9ZQsaJAgBEiB/QP+9Lb0YOhSQmIr/"
                                         "X75Vs1FME1qzmohSzqBVTzbfZFCnf1jsR2AAaiEFPxj3VK+"
                                         "knGrndOjcgMXI4wEfH/0VrbgJqobGWbewYyA=")));
        QCOMPARE(resultDeviceAlice.unrespondedSentStanzasCount, 10);
        QCOMPARE(resultDeviceAlice.unrespondedReceivedStanzasCount, 11);
        QCOMPARE(resultDeviceAlice.removalFromDeviceListDate, QDateTime(QDate(2022, 01, 01), QTime()));

        auto resultDevicesBob = result.value(QStringLiteral("bob@example.com"));
        QCOMPARE(resultDevicesBob.size(), 1);

        auto resultDeviceBob1 = resultDevicesBob.value(1);
        QCOMPARE(resultDeviceBob1.label, QStringLiteral("Phone"));
        QCOMPARE(resultDeviceBob1.keyId, QByteArray::fromBase64(("WTV6c3B2UFhYbE9OQ1d0N0ZScUhLWXpmYnY2emJoego=")));
        QCOMPARE(resultDeviceBob1.session,
                 QByteArray::fromBase64(("CvgCCAQSIQXZwE+"
                                         "G9R6ECMxKWPMidwcx3lPboUT2KEoea3B2T3vjUBohBQ7qW+Fb9Gi/"
                                         "SLsuQTv2TRixF0zLx2/"
                                         "mw0V4arjYSmgHIiCwuvEP2eyFU7FsbtSZBWKt+hH/"
                                         "DwBF7C0WrfxDrSu1bSgAMmsKIQXm5tRa73ZcUWn7fQa2YlDv+"
                                         "yLw1copPjdRZCrGcK7cNRIg0OXBvqBTAfyiUlLKW3LDIiSMHkRYYWDy"
                                         "knSJz3s+81oaJAgAEiAQlSKV+70EMYAjjW88dO52dp9e/"
                                         "aDhT8YUDHNFaCFUxTpJCiEF2OE4fb7Quwg0PMeJfT1uXmq/"
                                         "YXVaos9A7bn37TySiWkaJAgAEiDJlr5w0mBHBHZzttfVyvd2y2IzBV7"
                                         "bGdoX+"
                                         "lKHaEGIoUonCAwSIQXN7Y76Vwcsaubw8EHYaIPnBB11WjEEYcEPalwl"
                                         "gEUECRgCUMgnWMgnYABqIQXN7Y76Vwcsaubw8EHYaIPnBB11WjEEYcE"
                                         "PalwlgEUECQ==")));
        QCOMPARE(resultDeviceBob1.unrespondedSentStanzasCount, 20);
        QCOMPARE(resultDeviceBob1.unrespondedReceivedStanzasCount, 21);
        QCOMPARE(resultDeviceBob1.removalFromDeviceListDate, QDateTime(QDate(2022, 01, 02), QTime()));
    }

    storage.removeDevice(QStringLiteral("alice@example.org"), 1);

    {
        auto result = wait(this, storage.allData()).devices;
        QCOMPARE(result.size(), 1);

        auto resultDevicesBob = result.value(QStringLiteral("bob@example.com"));
        QCOMPARE(resultDevicesBob.size(), 1);

        auto resultDeviceBob1 = resultDevicesBob.value(1);
        QCOMPARE(resultDeviceBob1.label, QStringLiteral("Phone"));
        QCOMPARE(resultDeviceBob1.keyId, QByteArray::fromBase64(("WTV6c3B2UFhYbE9OQ1d0N0ZScUhLWXpmYnY2emJoego=")));
        QCOMPARE(resultDeviceBob1.session,
                 QByteArray::fromBase64(("CvgCCAQSIQXZwE+"
                                         "G9R6ECMxKWPMidwcx3lPboUT2KEoea3B2T3vjUBohBQ7qW+Fb9Gi/"
                                         "SLsuQTv2TRixF0zLx2/"
                                         "mw0V4arjYSmgHIiCwuvEP2eyFU7FsbtSZBWKt+hH/"
                                         "DwBF7C0WrfxDrSu1bSgAMmsKIQXm5tRa73ZcUWn7fQa2YlDv+"
                                         "yLw1copPjdRZCrGcK7cNRIg0OXBvqBTAfyiUlLKW3LDIiSMHkRYYWDy"
                                         "knSJz3s+81oaJAgAEiAQlSKV+70EMYAjjW88dO52dp9e/"
                                         "aDhT8YUDHNFaCFUxTpJCiEF2OE4fb7Quwg0PMeJfT1uXmq/"
                                         "YXVaos9A7bn37TySiWkaJAgAEiDJlr5w0mBHBHZzttfVyvd2y2IzBV7"
                                         "bGdoX+"
                                         "lKHaEGIoUonCAwSIQXN7Y76Vwcsaubw8EHYaIPnBB11WjEEYcEPalwl"
                                         "gEUECRgCUMgnWMgnYABqIQXN7Y76Vwcsaubw8EHYaIPnBB11WjEEYcE"
                                         "PalwlgEUECQ==")));
        QCOMPARE(resultDeviceBob1.unrespondedSentStanzasCount, 20);
        QCOMPARE(resultDeviceBob1.unrespondedReceivedStanzasCount, 21);
        QCOMPARE(resultDeviceBob1.removalFromDeviceListDate, QDateTime(QDate(2022, 01, 02), QTime()));
    }

    storage.addDevice(QStringLiteral("alice@example.org"), 1, deviceAlice);
    storage.addDevice(QStringLiteral("bob@example.com"), 2, deviceBob2);
    storage.removeDevices(QStringLiteral("bob@example.com"));

    {
        auto result = wait(this, storage.allData()).devices;
        QCOMPARE(result.size(), 1);

        auto resultDevicesAlice = result.value(QStringLiteral("alice@example.org"));
        QCOMPARE(resultDevicesAlice.size(), 1);

        auto resultDeviceAlice = resultDevicesAlice.value(1);
        QCOMPARE(resultDeviceAlice.label, QStringLiteral("Desktop"));
        QCOMPARE(resultDeviceAlice.keyId, QByteArray::fromBase64(("bEFLaDRQRkFlYXdyakE2aURoN0wyMzk2NTJEM2hRMgo=")));
        QCOMPARE(resultDeviceAlice.session,
                 QByteArray::fromBase64(("Cs8CCAQSIQWIhBRMdJ80tLVT7ius0H1LutRLeXBid68NH90M/"
                                         "kwhGxohBT+2kM/"
                                         "wVQ2UrZZPJBRmGZP0ZoCCWiET7KxA3ieAa888IiBSTWnp4qrTeo7z9k"
                                         "fKRaAFy+fYwPBI2HCSOxfC0anyPigAMmsKIQXZ95Xs7I+"
                                         "tOsg76eLtp266XTuCF8STa+"
                                         "VZkXPPJ00WSRIgmJ73wjhXPZqIt9ofB0NVwbWOKnYzQ90SHJEd/"
                                         "hyBHkUaJAgAEiDxXDT00+zpJd+TKJrD6nWQxQZhB8I7vCRdD/"
                                         "Oxw61MYjpJCiEFmTV1l+cOLEytoTp17VOEunYlCZmDqn/qoUYI/"
                                         "8P9ZQsaJAgBEiB/QP+9Lb0YOhSQmIr/"
                                         "X75Vs1FME1qzmohSzqBVTzbfZFCnf1jsR2AAaiEFPxj3VK+"
                                         "knGrndOjcgMXI4wEfH/0VrbgJqobGWbewYyA=")));
        QCOMPARE(resultDeviceAlice.unrespondedSentStanzasCount, 10);
        QCOMPARE(resultDeviceAlice.unrespondedReceivedStanzasCount, 11);
        QCOMPARE(resultDeviceAlice.removalFromDeviceListDate, QDateTime(QDate(2022, 01, 01), QTime()));
    }
}

void OmemoDbTest::testResetAll()
{
    storage.setOwnDevice(Storage::OwnDevice());

    Storage::SignedPreKeyPair signedPreKeyPair{
        .creationDate = QDateTime(QDate(2022, 01, 01), QTime()),
        .data = "FaZmWjwqppAoMff72qTzUIktGUbi4pAmds1Cuh6OElmi",
    };
    storage.addSignedPreKeyPair(1, signedPreKeyPair);

    storage.addPreKeyPairs({{1, ("RZLgD0lmL2WpJbskbGKFRMZL4zqSSvU0rElmO7UwGSVt")}, {2, ("3PGPNsf9P7pPitp9dt2uvZYT4HkxdHJAbWqLvOPXUeca")}});

    auto device = Storage::Device();
    device.keyId = "r4nd0m1d";
    storage.addDevice(QStringLiteral("alice@example.org"), 123, device);

    storage.resetAll();

    // check everything's empty
    auto data = wait(this, storage.allData());
    QCOMPARE(data.ownDevice, std::optional<Storage::OwnDevice>());
    QCOMPARE(data.signedPreKeyPairs, Storage::SignedPreKeyPairs());
    QCOMPARE(data.preKeyPairs, Storage::PreKeyPairs());
    QCOMPARE(data.devices, Storage::Devices());
}

QTEST_GUILESS_MAIN(OmemoDbTest)
#include "OmemoDbTest.moc"

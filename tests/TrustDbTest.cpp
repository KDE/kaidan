// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LGPL-2.1-or-later

#include <QXmppTrustLevel.h>

#include <QtTest>

#include "../src/Database.h"
#include "../src/FutureUtils.h"
#include "../src/TrustDb.h"
#include "utils.h"
#include <QXmppTrustMessageKeyOwner.h>

using namespace QXmpp;

constexpr auto ns_omemo = "urn:xmpp:omemo:2";
constexpr auto ns_ox = "urn:xmpp:ox:0";

class TrustDbTest : public QObject
{
	Q_OBJECT
	Q_SLOT void testSecurityPolicy();
	Q_SLOT void testOwnKeys();
	Q_SLOT void testKeys();
	Q_SLOT void testTrustLevels();
	Q_SLOT void testResetAll();
	Q_SLOT void atmTestKeysForPostponedTrustDecisions();
	Q_SLOT void atmTestResetAll();

private:
	Database db;
	TrustDb storage = TrustDb(&db, this, "account@example.org");
};

void TrustDbTest::testSecurityPolicy()
{
	QCOMPARE(wait(this, storage.securityPolicy(ns_ox)), QXmpp::NoSecurityPolicy);
	storage.setSecurityPolicy(ns_omemo, QXmpp::Toakafa);

	QCOMPARE(wait(this, storage.securityPolicy(ns_ox)), QXmpp::NoSecurityPolicy);
	QCOMPARE(wait(this, storage.securityPolicy(ns_omemo)), QXmpp::Toakafa);

	storage.resetSecurityPolicy(ns_omemo);
	QCOMPARE(wait(this, storage.securityPolicy(ns_omemo)), QXmpp::NoSecurityPolicy);
}

void TrustDbTest::testOwnKeys()
{
	QVERIFY(wait(this, storage.ownKey(ns_ox)).isNull());

	storage.setOwnKey(ns_ox, QByteArray::fromBase64(("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")));
	storage.setOwnKey(ns_omemo, QByteArray::fromBase64(("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")));

	// own OX key
	QCOMPARE(wait(this, storage.ownKey(ns_ox)),
		QByteArray::fromBase64(("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")));

	// own OMEMO key
	QCOMPARE(wait(this, storage.ownKey(ns_omemo)),
		QByteArray::fromBase64(("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")));

	storage.resetOwnKey(ns_omemo);

	// own OX key
	QCOMPARE(wait(this, storage.ownKey(ns_ox)),
		QByteArray::fromBase64(("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")));

	// no own OMEMO key
	QVERIFY(wait(this, storage.ownKey(ns_omemo)).isEmpty());
}

void TrustDbTest::testKeys()
{
	// no OMEMO keys
	QVERIFY(wait(this, storage.keys(ns_omemo)).isEmpty());

	// no OMEMO keys (via JIDs)
	QVERIFY(wait(this, storage.keys(ns_omemo, {"alice@example.org", "bob@example.com"})).isEmpty());

	// no automatically trusted and authenticated OMEMO keys
	QVERIFY(wait(this, storage.keys(ns_omemo, TrustLevel::AutomaticallyTrusted | TrustLevel::Authenticated))
			.isEmpty());

	// no automatically trusted and authenticated OMEMO key from Alice
	QVERIFY(!wait(this, storage.hasKey(
		ns_omemo, "alice@example.org", TrustLevel::AutomaticallyTrusted | TrustLevel::Authenticated)));

	// Store keys with the default trust level.
	storage.addKeys(ns_omemo,
		"alice@example.org",
		{QByteArray::fromBase64(("WaAnpWyW1hnFooH3oJo9Ba5XYoksnLPeJRTAjxPbv38=")),
			QByteArray::fromBase64(("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw="))});

	struct Key {
		QString encryption;
		QString jid;
		QByteArray key;
		TrustLevel trust;
	};
	auto addKey = [this](const Key &key) {
		wait(this, storage.addKeys(key.encryption, key.jid, {key.key}, key.trust));
	};
	std::vector<Key> keys = {
		Key {ns_omemo,
			"alice@example.org",
			QByteArray::fromBase64("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ="),
			TrustLevel::ManuallyDistrusted},
		Key {ns_omemo,
			"alice@example.org",
			QByteArray::fromBase64("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE="),
			TrustLevel::AutomaticallyTrusted},
		Key {ns_omemo,
			"bob@example.com",
			QByteArray::fromBase64("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg="),
			TrustLevel::AutomaticallyTrusted},
		Key {ns_omemo,
			"bob@example.com",
			QByteArray::fromBase64("2fhJtrgoMJxfLI3084/YkYh9paqiSiLFDVL2m0qAgX4="),
			TrustLevel::ManuallyTrusted},
		Key {ns_omemo,
			"bob@example.com",
			QByteArray::fromBase64("tCP1CI3pqSTVGzFYFyPYUMfMZ9Ck/msmfD0wH/VtJBM="),
			TrustLevel::ManuallyTrusted},
		Key {ns_omemo,
			"bob@example.com",
			QByteArray::fromBase64("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8="),
			TrustLevel::Authenticated},
		Key {ns_ox,
			"alice@example.org",
			QByteArray::fromBase64("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ="),
			TrustLevel::Authenticated},
		Key {ns_ox,
			"alice@example.org",
			QByteArray::fromBase64("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA="),
			TrustLevel::Authenticated},
	};
	std::for_each(keys.cbegin(), keys.cend(), addKey);

	QMultiHash<QString, QByteArray> automaticallyDistrustedKeys = {
		{"alice@example.org", QByteArray::fromBase64(("WaAnpWyW1hnFooH3oJo9Ba5XYoksnLPeJRTAjxPbv38="))},
		{"alice@example.org", QByteArray::fromBase64(("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw="))}};
	QMultiHash<QString, QByteArray> manuallyDistrustedKeys = {{"alice@example.org",
		QByteArray::fromBase64(("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ="))}};
	QMultiHash<QString, QByteArray> automaticallyTrustedKeys = {
		{"alice@example.org", QByteArray::fromBase64(("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE="))},
		{"bob@example.com", QByteArray::fromBase64(("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg="))}};
	QMultiHash<QString, QByteArray> manuallyTrustedKeys = {
		{"bob@example.com", QByteArray::fromBase64(("tCP1CI3pqSTVGzFYFyPYUMfMZ9Ck/msmfD0wH/VtJBM="))},
		{"bob@example.com", QByteArray::fromBase64(("2fhJtrgoMJxfLI3084/YkYh9paqiSiLFDVL2m0qAgX4="))}};
	QMultiHash<QString, QByteArray> authenticatedKeys = {{"bob@example.com",
		QByteArray::fromBase64(("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8="))}};

	QHash<QByteArray, TrustLevel> keysAlice = {
		{QByteArray::fromBase64("WaAnpWyW1hnFooH3oJo9Ba5XYoksnLPeJRTAjxPbv38="), TrustLevel::AutomaticallyDistrusted},
		{QByteArray::fromBase64("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw="), TrustLevel::AutomaticallyDistrusted},
		{QByteArray::fromBase64("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ="), TrustLevel::ManuallyDistrusted},
		{QByteArray::fromBase64("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE="),
			TrustLevel::AutomaticallyTrusted}};
	QHash<QByteArray, TrustLevel> keysBob = {
		{QByteArray::fromBase64("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg="), TrustLevel::AutomaticallyTrusted},
		{QByteArray::fromBase64("tCP1CI3pqSTVGzFYFyPYUMfMZ9Ck/msmfD0wH/VtJBM="), TrustLevel::ManuallyTrusted},
		{QByteArray::fromBase64("2fhJtrgoMJxfLI3084/YkYh9paqiSiLFDVL2m0qAgX4="), TrustLevel::ManuallyTrusted},
		{QByteArray::fromBase64("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8="), TrustLevel::Authenticated}};

	// all OMEMO keys
	auto omemoKeys = wait(this, storage.keys(ns_omemo));
	QCOMPARE(omemoKeys[TrustLevel::AutomaticallyDistrusted], automaticallyDistrustedKeys);
	QCOMPARE(omemoKeys[TrustLevel::ManuallyDistrusted], manuallyDistrustedKeys);
	QCOMPARE(omemoKeys[TrustLevel::AutomaticallyTrusted], automaticallyTrustedKeys);
	QCOMPARE(omemoKeys[TrustLevel::ManuallyTrusted], manuallyTrustedKeys);
	QCOMPARE(omemoKeys[TrustLevel::Authenticated], authenticatedKeys);

	// automatically trusted and authenticated OMEMO keys
	QCOMPARE(wait(this, storage.keys(ns_omemo, TrustLevel::AutomaticallyTrusted | TrustLevel::Authenticated)),
		QHash({std::pair(TrustLevel::AutomaticallyTrusted, automaticallyTrustedKeys),
			std::pair(TrustLevel::Authenticated, authenticatedKeys)}));

	// all OMEMO keys (via JIDs)
	QCOMPARE(wait(this, storage.keys(ns_omemo, {"alice@example.org", "bob@example.com"})),
		TrustDb::KeysByOwner({std::pair("alice@example.org", keysAlice),
			std::pair("bob@example.com", keysBob)}));

	// Alice's OMEMO keys
	QCOMPARE(wait(this, storage.keys(ns_omemo, {"alice@example.org"})),
		TrustDb::KeysByOwner({std::pair("alice@example.org", keysAlice)}));

	keysAlice = {{QByteArray::fromBase64(("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE=")),
		TrustLevel::AutomaticallyTrusted}};
	keysBob = {{QByteArray::fromBase64(("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")),
			   TrustLevel::AutomaticallyTrusted},
		{QByteArray::fromBase64(("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8=")), TrustLevel::Authenticated}};

	// automatically trusted and authenticated OMEMO keys (via JIDs)
	QCOMPARE(wait(this, storage.keys(ns_omemo,
			 {"alice@example.org", "bob@example.com"},
			 TrustLevel::AutomaticallyTrusted | TrustLevel::Authenticated)),
		TrustDb::KeysByOwner({std::pair("alice@example.org", keysAlice),
			std::pair("bob@example.com", keysBob)}));

	// Alice's automatically trusted and authenticated OMEMO keys
	QCOMPARE(wait(this, storage.keys(ns_omemo, {"alice@example.org"}, TrustLevel::AutomaticallyTrusted | TrustLevel::Authenticated)),
		TrustDb::KeysByOwner({std::pair("alice@example.org", keysAlice)}));

	// at least one automatically trusted or authenticated OMEMO key from Alice
	QVERIFY(wait(this, storage.hasKey(
		ns_omemo, "alice@example.org", TrustLevel::AutomaticallyTrusted | TrustLevel::Authenticated)));

	storage.removeKeys(ns_omemo,
		{QByteArray::fromBase64("WaAnpWyW1hnFooH3oJo9Ba5XYoksnLPeJRTAjxPbv38="),
			QByteArray::fromBase64("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE=")});

	automaticallyDistrustedKeys = {{"alice@example.org",
		QByteArray::fromBase64(("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw="))}};
	automaticallyTrustedKeys = {{"bob@example.com",
		QByteArray::fromBase64(("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg="))}};

	keysAlice = {{QByteArray::fromBase64(("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw=")),
			     TrustLevel::AutomaticallyDistrusted},
		{QByteArray::fromBase64(("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")),
			TrustLevel::ManuallyDistrusted}};
	keysBob = {{QByteArray::fromBase64(("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")),
			   TrustLevel::AutomaticallyTrusted},
		{QByteArray::fromBase64(("tCP1CI3pqSTVGzFYFyPYUMfMZ9Ck/msmfD0wH/VtJBM=")), TrustLevel::ManuallyTrusted},
		{QByteArray::fromBase64(("2fhJtrgoMJxfLI3084/YkYh9paqiSiLFDVL2m0qAgX4=")), TrustLevel::ManuallyTrusted},
		{QByteArray::fromBase64(("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8=")), TrustLevel::Authenticated}};

	// OMEMO keys after removal
	QCOMPARE(wait(this, storage.keys(ns_omemo)),
		QHash({std::pair(TrustLevel::AutomaticallyDistrusted, automaticallyDistrustedKeys),
			std::pair(TrustLevel::ManuallyDistrusted, manuallyDistrustedKeys),
			std::pair(TrustLevel::AutomaticallyTrusted, automaticallyTrustedKeys),
			std::pair(TrustLevel::ManuallyTrusted, manuallyTrustedKeys),
			std::pair(TrustLevel::Authenticated, authenticatedKeys)}));

	// OMEMO keys after removal (via JIDs)
	QCOMPARE(wait(this, storage.keys(ns_omemo, {"alice@example.org", "bob@example.com"})),
		TrustDb::KeysByOwner({std::pair("alice@example.org", keysAlice),
			std::pair("bob@example.com", keysBob)}));

	// Alice's OMEMO keys after removal
	QCOMPARE(wait(this, storage.keys(ns_omemo, {"alice@example.org"})),
		TrustDb::KeysByOwner({std::pair("alice@example.org", keysAlice)}));

	keysAlice = {{QByteArray::fromBase64(("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE=")),
		TrustLevel::AutomaticallyTrusted}};
	keysBob = {{QByteArray::fromBase64(("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")),
			   TrustLevel::AutomaticallyTrusted},
		{QByteArray::fromBase64(("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8=")), TrustLevel::Authenticated}};

	// automatically trusted and authenticated OMEMO keys after removal (via JIDs)
	QCOMPARE(wait(this, storage.keys(ns_omemo,
			 {"alice@example.org", "bob@example.com"},
			 TrustLevel::AutomaticallyTrusted | TrustLevel::Authenticated)),
		TrustDb::KeysByOwner({std::pair("bob@example.com", keysBob)}));

	// Alice's automatically trusted and authenticated OMEMO keys after removal
	QVERIFY(wait(this, storage.keys(ns_omemo, {"alice@example.org"}, TrustLevel::AutomaticallyTrusted | TrustLevel::Authenticated))
			.isEmpty());

	storage.removeKeys(ns_omemo, "alice@example.org");

	// OMEMO keys after removing Alice's keys
	QCOMPARE(wait(this, storage.keys(ns_omemo)),
		QHash({std::pair(TrustLevel::AutomaticallyTrusted, automaticallyTrustedKeys),
			std::pair(TrustLevel::ManuallyTrusted, manuallyTrustedKeys),
			std::pair(TrustLevel::Authenticated, authenticatedKeys)}));

	storage.removeKeys(ns_omemo);

	// no stored OMEMO keys
	QVERIFY(wait(this, storage.keys(ns_omemo)).isEmpty());

	authenticatedKeys = {
		{"alice@example.org", QByteArray::fromBase64(("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ="))},
		{"alice@example.org", QByteArray::fromBase64(("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA="))}};

	keysAlice = {{QByteArray::fromBase64(("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")),
			     TrustLevel::Authenticated},
		{QByteArray::fromBase64(("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")), TrustLevel::Authenticated}};

	// remaining OX keys
	QCOMPARE(wait(this, storage.keys(ns_ox)), QHash({std::pair(TrustLevel::Authenticated, authenticatedKeys)}));

	// remaining OX keys (via JIDs)
	QCOMPARE(wait(this, storage.keys(ns_ox, {"alice@example.org", "bob@example.com"})),
		TrustDb::KeysByOwner({std::pair("alice@example.org", keysAlice)}));

	// Alice's remaining OX keys
	QCOMPARE(wait(this, storage.keys(ns_ox, {"alice@example.org"})),
		TrustDb::KeysByOwner({{"alice@example.org", keysAlice}}));

	storage.removeKeys(ns_ox);

	// no stored OX keys
	QVERIFY(wait(this, storage.keys(ns_ox)).isEmpty());

	// no stored OX keys (via JIDs)
	QVERIFY(wait(this, storage.keys(ns_ox, {"alice@example.org", "bob@example.com"})).isEmpty());

	// no automatically trusted or authenticated OX key from Alice
	auto hasKeyResult = wait(this, storage.hasKey(
		ns_ox, "alice@example.org", TrustLevel::AutomaticallyTrusted | TrustLevel::Authenticated));
	QVERIFY(!hasKeyResult);
}

void TrustDbTest::testTrustLevels()
{
	storage.addKeys(ns_ox,
		"alice@example.org",
		{QByteArray::fromBase64(("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU="))},
		TrustLevel::AutomaticallyTrusted);

	storage.addKeys(ns_omemo,
		"alice@example.org",
		{QByteArray::fromBase64(("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU=")),
			QByteArray::fromBase64(("JU4pT7Ivpigtl+7QE87Bkq4r/C/mhI1FCjY5Wmjbtwg="))},
		TrustLevel::AutomaticallyTrusted);

	storage.addKeys(ns_omemo,
		"alice@example.org",
		{QByteArray::fromBase64(("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ="))},
		TrustLevel::ManuallyTrusted);

	storage.addKeys(ns_omemo,
		"bob@example.com",
		{QByteArray::fromBase64(("9E51lG3vVmUn8CM7/AIcmIlLP2HPl6Ao0/VSf4VT/oA="))},
		TrustLevel::AutomaticallyTrusted);

	QCOMPARE(wait(this, storage.trustLevel(ns_omemo,
			 "alice@example.org",
			 QByteArray::fromBase64(("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU=")))),
		TrustLevel::AutomaticallyTrusted);

	storage.setTrustLevel(ns_omemo,
		{{"alice@example.org", QByteArray::fromBase64(("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU="))},
			{"bob@example.com", QByteArray::fromBase64(("9E51lG3vVmUn8CM7/AIcmIlLP2HPl6Ao0/VSf4VT/oA="))}},
		TrustLevel::Authenticated);

	QCOMPARE(int(wait(this, storage.trustLevel(ns_omemo,
			 "alice@example.org",
			 QByteArray::fromBase64("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU=")))),
		int(TrustLevel::Authenticated));

	QCOMPARE(wait(this, storage.trustLevel(ns_omemo,
			 "bob@example.com",
			 QByteArray::fromBase64(("9E51lG3vVmUn8CM7/AIcmIlLP2HPl6Ao0/VSf4VT/oA=")))),
		TrustLevel::Authenticated);

	// Set the trust level of a key that is not stored yet.
	// It is added to the storage automatically.
	wait(this, storage.setTrustLevel(ns_omemo,
		{{"alice@example.org", QByteArray::fromBase64("9w6oPjKyGSALd9gHq7sNOdOAkD5bHUVOKACNs89FjkA=")}},
		TrustLevel::ManuallyTrusted));

	QCOMPARE(int(wait(this, storage.trustLevel(ns_omemo,
			 "alice@example.org",
			 QByteArray::fromBase64("9w6oPjKyGSALd9gHq7sNOdOAkD5bHUVOKACNs89FjkA=")))),
		int(TrustLevel::ManuallyTrusted));

	// Try to retrieve the trust level of a key that is not stored yet.
	// The default value is returned.
	QCOMPARE(wait(this, storage.trustLevel(ns_omemo,
			 "someone@example.org",
			 QByteArray::fromBase64("WXL4EDfzUGbVPQWjT9pmBeiCpCBzYZv3lUAaj+UbPyE="))),
		TrustLevel::Undecided);

	// Set the trust levels of all authenticated keys belonging to Alice and
	// Bob.
	storage.setTrustLevel(ns_omemo,
		{"alice@example.org", "bob@example.com"},
		TrustLevel::Authenticated,
		TrustLevel::ManuallyDistrusted);

	QCOMPARE(wait(this, storage.trustLevel(ns_omemo,
			 "alice@example.org",
			 QByteArray::fromBase64(("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU=")))),
		TrustLevel::ManuallyDistrusted);

	QCOMPARE(wait(this, storage.trustLevel(ns_omemo,
			 "bob@example.com",
			 QByteArray::fromBase64("9E51lG3vVmUn8CM7/AIcmIlLP2HPl6Ao0/VSf4VT/oA="))),
		TrustLevel::ManuallyDistrusted);

	// Verify that the default trust level is returned for an unknown key.
	QCOMPARE(wait(this, storage.trustLevel(ns_omemo,
			 "somebody@example.org",
			 QByteArray::fromBase64(("wE06Gwf8f4DvDLFDoaCsGs8ibcUjf84WIOA2FAjPI3o=")))),
		TrustLevel::Undecided);

	storage.removeKeys(ns_ox);
	storage.removeKeys(ns_omemo);
}

void TrustDbTest::testResetAll()
{
	storage.setSecurityPolicy(ns_ox, Toakafa);
	storage.setSecurityPolicy(ns_omemo, Toakafa);

	storage.setOwnKey(ns_ox, QByteArray::fromBase64(("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")));
	storage.setOwnKey(ns_omemo, QByteArray::fromBase64(("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")));

	storage.addKeys(ns_omemo,
		"alice@example.org",
		{QByteArray::fromBase64("WaAnpWyW1hnFooH3oJo9Ba5XYoksnLPeJRTAjxPbv38="),
			QByteArray::fromBase64("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw=")});

	storage.addKeys(ns_omemo,
		"alice@example.org",
		{QByteArray::fromBase64(("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ="))},
		TrustLevel::ManuallyDistrusted);

	storage.addKeys(ns_omemo,
		"alice@example.org",
		{QByteArray::fromBase64(("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE="))},
		TrustLevel::AutomaticallyTrusted);

	storage.addKeys(ns_omemo,
		"bob@example.com",
		{QByteArray::fromBase64(("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg="))},
		TrustLevel::AutomaticallyTrusted);

	storage.addKeys(ns_omemo,
		"bob@example.com",
		{QByteArray::fromBase64(("tCP1CI3pqSTVGzFYFyPYUMfMZ9Ck/msmfD0wH/VtJBM=")),
			QByteArray::fromBase64(("2fhJtrgoMJxfLI3084/"
						"YkYh9paqiSiLFDVL2m0qAgX4="))},
		TrustLevel::ManuallyTrusted);

	storage.addKeys(ns_omemo,
		"bob@example.com",
		{QByteArray::fromBase64(("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8="))},
		TrustLevel::Authenticated);

	storage.addKeys(ns_ox,
		"alice@example.org",
		{QByteArray::fromBase64("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ="),
			QByteArray::fromBase64("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")},
		TrustLevel::Authenticated);

	storage.resetAll(ns_omemo);
	QCOMPARE(wait(this, storage.securityPolicy(ns_omemo)), NoSecurityPolicy);
	QCOMPARE(wait(this, storage.securityPolicy(ns_ox)), Toakafa);
	QVERIFY(wait(this, storage.ownKey(ns_omemo)).isEmpty());
	QCOMPARE(wait(this, storage.ownKey(ns_ox)),
		QByteArray::fromBase64("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ="));
	QVERIFY(wait(this, storage.keys(ns_omemo)).isEmpty());

	const QMultiHash<QString, QByteArray> authenticatedKeys = {
		{"alice@example.org", QByteArray::fromBase64("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")},
		{"alice@example.org", QByteArray::fromBase64("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")}};

	QCOMPARE(wait(this, storage.keys(ns_ox)), QHash({std::pair {TrustLevel::Authenticated, authenticatedKeys}}));

	return;
	storage.resetAll(ns_ox);
	storage.resetAll(ns_omemo);
}

void TrustDbTest::atmTestKeysForPostponedTrustDecisions()
{
	// The key 7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU=
	// is set for both postponed authentication and distrusting.
	// Thus, it is only stored for postponed distrusting.
	QXmppTrustMessageKeyOwner keyOwnerAlice;
	keyOwnerAlice.setJid("alice@example.org");
	keyOwnerAlice.setTrustedKeys(
		{QByteArray::fromBase64(("Wl53ZchbtAtCZQCHROiD20W7UnKTQgWQrjTHAVNw1ic=")),
			QByteArray::fromBase64(("QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE=")),
			QByteArray::fromBase64(("7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU="))});
	keyOwnerAlice.setDistrustedKeys(
		{QByteArray::fromBase64(("mB98hhdVps++skUuy4TGy/Vp6RQXLJO4JGf86FAUjyc=")),
			QByteArray::fromBase64(("7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU="))});

	QXmppTrustMessageKeyOwner keyOwnerBobTrustedKeys;
	keyOwnerBobTrustedKeys.setJid("bob@example.com");
	keyOwnerBobTrustedKeys.setTrustedKeys(
		{QByteArray::fromBase64(("GgTqeRLp1M+MEenzFQym2oqer9PfHukS4brJDQl5ARE="))});

	storage.addKeysForPostponedTrustDecisions(ns_omemo,
		QByteArray::fromBase64(("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE=")),
		{keyOwnerAlice, keyOwnerBobTrustedKeys});

	QXmppTrustMessageKeyOwner keyOwnerBobDistrustedKeys;
	keyOwnerBobDistrustedKeys.setJid("bob@example.com");
	keyOwnerBobDistrustedKeys.setDistrustedKeys(
		{QByteArray::fromBase64(("sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA=")),
			QByteArray::fromBase64(("X5tJ1D5rEeaeQE8eqhBKAj4KUZGYe3x+iHifaTBY1kM="))});

	storage.addKeysForPostponedTrustDecisions(ns_omemo,
		QByteArray::fromBase64(("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=")),
		{keyOwnerBobDistrustedKeys});

	QXmppTrustMessageKeyOwner keyOwnerCarol;
	keyOwnerCarol.setJid(("carol@example.net"));
	keyOwnerCarol.setTrustedKeys(
		{QByteArray::fromBase64(("WcL+cEMpEeK+dpqg3Xd3amctzwP8h2MqwXcEzFf6LpU=")),
			QByteArray::fromBase64(("bH3R31z0N97K1fUwG3+bdBrVPuDfXguQapHudkfa5nE="))});
	keyOwnerCarol.setDistrustedKeys(
		{QByteArray::fromBase64(("N0B2StHKk1/slwg1rzybTFzjdg7FChc+3cXmTU/rS8g=")),
			QByteArray::fromBase64(("wsEN32UHCiNjYqTG/J63hY4Nu8tZT42Ni1FxrgyRQ5g="))});

	storage.addKeysForPostponedTrustDecisions(ns_ox,
		QByteArray::fromBase64(("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=")),
		{keyOwnerCarol});

	QMultiHash<QString, QByteArray> trustedKeys = {
		{"alice@example.org", QByteArray::fromBase64(("Wl53ZchbtAtCZQCHROiD20W7UnKTQgWQrjTHAVNw1ic="))},
		{"alice@example.org", QByteArray::fromBase64(("QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE="))},
		{"bob@example.com", QByteArray::fromBase64(("GgTqeRLp1M+MEenzFQym2oqer9PfHukS4brJDQl5ARE="))}};
	QMultiHash<QString, QByteArray> distrustedKeys = {
		{"alice@example.org", QByteArray::fromBase64(("mB98hhdVps++skUuy4TGy/Vp6RQXLJO4JGf86FAUjyc="))},
		{"alice@example.org", QByteArray::fromBase64(("7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU="))}};

	QCOMPARE(wait(this, storage.keysForPostponedTrustDecisions(
			 ns_omemo, {QByteArray::fromBase64(("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE="))})),
		QHash({std::pair(true, trustedKeys), std::pair(false, distrustedKeys)}));

	distrustedKeys = {{"alice@example.org", QByteArray::fromBase64(("mB98hhdVps++skUuy4TGy/Vp6RQXLJO4JGf86FAUjyc="))},
		{"alice@example.org", QByteArray::fromBase64(("7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU="))},
		{"bob@example.com", QByteArray::fromBase64(("sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA="))},
		{"bob@example.com", QByteArray::fromBase64(("X5tJ1D5rEeaeQE8eqhBKAj4KUZGYe3x+iHifaTBY1kM="))}};

	QCOMPARE(wait(this, storage.keysForPostponedTrustDecisions(ns_omemo,
			 {QByteArray::fromBase64(("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE=")),
				 QByteArray::fromBase64(("IL5iwDQwquH7yjb5RAiIP+"
							 "nvYiBUsNCXtKB8IpKc9QU="))})),
		QHash({std::pair(true, trustedKeys), std::pair(false, distrustedKeys)}));

	// Retrieve all keys.
	QCOMPARE(wait(this, storage.keysForPostponedTrustDecisions(ns_omemo)),
		QHash({std::pair(true, trustedKeys), std::pair(false, distrustedKeys)}));

	keyOwnerBobTrustedKeys.setTrustedKeys(
		{QByteArray::fromBase64(("sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA="))});

	// Invert the trust in Bob's key
	// sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA= for the
	// sending endpoint with the key
	// IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=.
	storage.addKeysForPostponedTrustDecisions(ns_omemo,
		QByteArray::fromBase64(("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=")),
		{keyOwnerBobTrustedKeys});

	trustedKeys = {{"bob@example.com",
		QByteArray::fromBase64(("sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA="))}};
	distrustedKeys = {{"bob@example.com",
		QByteArray::fromBase64(("X5tJ1D5rEeaeQE8eqhBKAj4KUZGYe3x+iHifaTBY1kM="))}};

	QCOMPARE(wait(this, storage.keysForPostponedTrustDecisions(
			 ns_omemo, {QByteArray::fromBase64(("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU="))})),
		QHash({std::pair(true, trustedKeys), std::pair(false, distrustedKeys)}));

	storage.removeKeysForPostponedTrustDecisions(ns_omemo,
		{QByteArray::fromBase64(("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE="))});

	QCOMPARE(wait(this, storage.keysForPostponedTrustDecisions(ns_omemo)),
		QHash({std::pair(true, trustedKeys), std::pair(false, distrustedKeys)}));

	storage.addKeysForPostponedTrustDecisions(ns_omemo,
		QByteArray::fromBase64(("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE=")),
		{keyOwnerAlice});

	// The key QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE= is not removed
	// because its ID is passed within the parameter "keyIdsForDistrusting" but
	// stored for postponed authentication.
	storage.removeKeysForPostponedTrustDecisions(ns_omemo,
		{QByteArray::fromBase64(("Wl53ZchbtAtCZQCHROiD20W7UnKTQgWQrjTHAVNw1ic=")),
			QByteArray::fromBase64(("sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+"
						"4RpsxA="))},
		{QByteArray::fromBase64(("mB98hhdVps++skUuy4TGy/Vp6RQXLJO4JGf86FAUjyc=")),
			QByteArray::fromBase64(("QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE"
						"="))});

	trustedKeys = {{"alice@example.org",
		QByteArray::fromBase64(("QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE="))}};
	distrustedKeys = {{"alice@example.org", QByteArray::fromBase64(("7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU="))},
		{"bob@example.com", QByteArray::fromBase64(("X5tJ1D5rEeaeQE8eqhBKAj4KUZGYe3x+iHifaTBY1kM="))}};

	QCOMPARE(wait(this, storage.keysForPostponedTrustDecisions(ns_omemo)),
		QHash({std::pair(true, trustedKeys), std::pair(false, distrustedKeys)}));

	// Remove all OMEMO keys.
	storage.removeKeysForPostponedTrustDecisions(ns_omemo);

	QVERIFY(wait(this, storage.keysForPostponedTrustDecisions(ns_omemo)).isEmpty());

	trustedKeys = {{("carol@example.net"), QByteArray::fromBase64(("WcL+cEMpEeK+dpqg3Xd3amctzwP8h2MqwXcEzFf6LpU="))},
		{("carol@example.net"), QByteArray::fromBase64(("bH3R31z0N97K1fUwG3+bdBrVPuDfXguQapHudkfa5nE="))}};
	distrustedKeys = {{("carol@example.net"), QByteArray::fromBase64(("N0B2StHKk1/slwg1rzybTFzjdg7FChc+3cXmTU/rS8g="))},
		{("carol@example.net"), QByteArray::fromBase64(("wsEN32UHCiNjYqTG/J63hY4Nu8tZT42Ni1FxrgyRQ5g="))}};

	// remaining OX keys
	QCOMPARE(wait(this, storage.keysForPostponedTrustDecisions(ns_ox)),
		QHash({std::pair(true, trustedKeys), std::pair(false, distrustedKeys)}));

	storage.removeKeysForPostponedTrustDecisions(ns_ox);

	// no OX keys
	QVERIFY(wait(this, storage.keysForPostponedTrustDecisions(ns_ox)).isEmpty());
}

void TrustDbTest::atmTestResetAll()
{
	QXmppTrustMessageKeyOwner keyOwnerAlice;
	keyOwnerAlice.setJid("alice@example.org");
	keyOwnerAlice.setTrustedKeys(
		{QByteArray::fromBase64(("Wl53ZchbtAtCZQCHROiD20W7UnKTQgWQrjTHAVNw1ic=")),
			QByteArray::fromBase64(("QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE="))});
	keyOwnerAlice.setDistrustedKeys(
		{QByteArray::fromBase64(("mB98hhdVps++skUuy4TGy/Vp6RQXLJO4JGf86FAUjyc="))});

	QXmppTrustMessageKeyOwner keyOwnerBobTrustedKeys;
	keyOwnerBobTrustedKeys.setJid("bob@example.com");
	keyOwnerBobTrustedKeys.setTrustedKeys(
		{QByteArray::fromBase64(("GgTqeRLp1M+MEenzFQym2oqer9PfHukS4brJDQl5ARE="))});

	storage.addKeysForPostponedTrustDecisions(ns_omemo,
		QByteArray::fromBase64(("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE=")),
		{keyOwnerAlice, keyOwnerBobTrustedKeys});

	QXmppTrustMessageKeyOwner keyOwnerCarol;
	keyOwnerCarol.setJid(("carol@example.net"));
	keyOwnerCarol.setTrustedKeys(
		{QByteArray::fromBase64(("WcL+cEMpEeK+dpqg3Xd3amctzwP8h2MqwXcEzFf6LpU=")),
			QByteArray::fromBase64(("bH3R31z0N97K1fUwG3+bdBrVPuDfXguQapHudkfa5nE="))});
	keyOwnerCarol.setDistrustedKeys(
		{QByteArray::fromBase64(("N0B2StHKk1/slwg1rzybTFzjdg7FChc+3cXmTU/rS8g=")),
			QByteArray::fromBase64(("wsEN32UHCiNjYqTG/J63hY4Nu8tZT42Ni1FxrgyRQ5g="))});

	storage.addKeysForPostponedTrustDecisions(ns_ox,
		QByteArray::fromBase64(("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=")),
		{keyOwnerCarol});

	storage.resetAll(ns_omemo);
	QVERIFY(wait(this, storage.keysForPostponedTrustDecisions(ns_omemo)).isEmpty());

	QMultiHash<QString, QByteArray> trustedKeys = {
		{("carol@example.net"), QByteArray::fromBase64(("WcL+cEMpEeK+dpqg3Xd3amctzwP8h2MqwXcEzFf6LpU="))},
		{("carol@example.net"), QByteArray::fromBase64(("bH3R31z0N97K1fUwG3+bdBrVPuDfXguQapHudkfa5nE="))}};
	QMultiHash<QString, QByteArray> distrustedKeys = {
		{("carol@example.net"), QByteArray::fromBase64(("N0B2StHKk1/slwg1rzybTFzjdg7FChc+3cXmTU/rS8g="))},
		{("carol@example.net"), QByteArray::fromBase64(("wsEN32UHCiNjYqTG/J63hY4Nu8tZT42Ni1FxrgyRQ5g="))}};

	QCOMPARE(wait(this, storage.keysForPostponedTrustDecisions(ns_ox)),
		QHash({std::pair(true, trustedKeys), std::pair(false, distrustedKeys)}));
}

QTEST_GUILESS_MAIN(TrustDbTest)
#include "TrustDbTest.moc"

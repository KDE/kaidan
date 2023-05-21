// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt
#include <QRandomGenerator>
// Kaidan
#include "CredentialsGenerator.h"
#include "Globals.h"

static const QLatin1String VOWELS = QLatin1String("aeiou");
static const int VOWELS_LENGTH = VOWELS.size();

static const QLatin1String CONSONANTS = QLatin1String("bcdfghjklmnpqrstvwxyz");
static const int CONSONANTS_LENGTH = CONSONANTS.size();

CredentialsGenerator::CredentialsGenerator(QObject *parent)
	: QObject(parent)
{
}

QString CredentialsGenerator::generatePronounceableName(unsigned int length)
{
	QString randomString;
	randomString.reserve(length);
	bool startWithVowel = QRandomGenerator::global()->generate() % 2;
	length += startWithVowel;
	for (unsigned int i = startWithVowel; i < length; ++i) {
		if (i % 2)
			randomString.append(VOWELS.at(QRandomGenerator::global()->generate() % VOWELS_LENGTH));
		else
			randomString.append(CONSONANTS.at(QRandomGenerator::global()->generate() % CONSONANTS_LENGTH));
	}
	return randomString;
}

QString CredentialsGenerator::generateUsername()
{
	return generatePronounceableName(GENERATED_USERNAME_LENGTH);
}

QString CredentialsGenerator::generatePassword()
{
	return generatePassword(GENERATED_PASSWORD_LENGTH_LOWER_BOUND + QRandomGenerator::global()->generate() % (GENERATED_PASSWORD_LENGTH_UPPER_BOUND - GENERATED_PASSWORD_LENGTH_LOWER_BOUND + 1));
}

QString CredentialsGenerator::generatePassword(unsigned int length)
{
	QString password;
	password.reserve(length);

	for (unsigned int i = 0; i < length; i++)
		password.append(GENERATED_PASSWORD_ALPHABET.at(QRandomGenerator::global()->generate() % GENERATED_PASSWORD_ALPHABET_LENGTH));

	return password;
}

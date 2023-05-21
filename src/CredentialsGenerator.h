// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

/**
 * This class contains generators for usernames and passwords.
 */
class CredentialsGenerator : public QObject
{
	Q_OBJECT

public:
	explicit CredentialsGenerator(QObject *parent = nullptr);

	/**
	 * Generates a random string with alternating consonants and vowels.
	 *
	 * Whether the string starts with a consonant or vowel is random.
	 */
	Q_INVOKABLE static QString generatePronounceableName(unsigned int length);

	/**
	 * Generates a pronounceable username with @c GENERATED_USERNAME_LENGTH as fixed length.
	 */
	Q_INVOKABLE static QString generateUsername();

	/**
	 * Generates a random password containing characters from
	 * @c GENERATED_PASSWORD_ALPHABET with a length between
	 * @c GENERATED_PASSWORD_LOWER_BOUND (including) and
	 * @c GENERATED_PASSWORD_UPPER_BOUND (including).
	 */
	Q_INVOKABLE static QString generatePassword();

	/**
	 * Generates a random password containing characters from
	 * @c GENERATED_PASSWORD_ALPHABET.
	 */
	Q_INVOKABLE static QString generatePassword(unsigned int length);
};

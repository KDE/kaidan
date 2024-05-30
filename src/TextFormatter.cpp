// SPDX-FileCopyrightText: 2021 Carson Black <uhhadd@gmail.com>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TextFormatter.h"

#include <unicode/uchar.h>
#include <QGuiApplication>
#include <QPalette>
#include <QQuickTextDocument>
#include <QTextBoundaryFinder>
#include <QTextCursor>
#include <QTextDocument>

#include "Algorithms.h"
#include "QmlUtils.h"

const auto EMOJI_FONT_FAMILY = QStringLiteral("emoji");
constexpr auto URL_PREFIX = u"https://";
constexpr auto SINGLE_EMOJI_SIZE_FACTOR = 3;
constexpr auto MULTIPLE_EMOJIS_SIZE_FACTOR = 2;
constexpr auto MIXED_TEXT_EMOJI_SIZE_FACTOR = 1.3;

enum class TextType {
	Mixed,
	SingleEmoji,
	MultipleEmojis,
};

static TextType determineTextType(const QString &text) {
	QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
	auto emojiCounter = 0;

	for (auto start = 0, end = 0; finder.toNextBoundary() != -1; start = end) {
		end = finder.position();
		auto part = text.mid(start, end - start).toUcs4()[0];
		const auto firstCodepoint = text[start];

		if (firstCodepoint.isSpace() || firstCodepoint == MESSAGE_BUBBLE_PADDING_CHARACTER) {
			continue;
		}

		if (u_hasBinaryProperty(part, UCHAR_EMOJI_PRESENTATION)) {
			emojiCounter++;
		} else {
			return TextType::Mixed;
		}
	}

	switch (emojiCounter) {
	case 0:
		return TextType::Mixed;
	case 1:
		return TextType::SingleEmoji;
	default:
		return TextType::MultipleEmojis;
	}
}

static double determineEmojiFontSizeFactor(const QString &text) {
	switch (determineTextType(text)) {
	case TextType::Mixed:
		return MIXED_TEXT_EMOJI_SIZE_FACTOR;
	case TextType::SingleEmoji:
		return SINGLE_EMOJI_SIZE_FACTOR;
	case TextType::MultipleEmojis:
		return MULTIPLE_EMOJIS_SIZE_FACTOR;
	default:
		return 1;
	}
}

static auto isTextSeparator(QChar character)
{
	return character.isSpace() || character == MESSAGE_BUBBLE_PADDING_CHARACTER;
}

// Formats emojis to use an appropriate emoji font that usually displays coloured emojis.
static void formatEmojis(QTextCursor &cursor, const QString &text, double emojiFontSizeFactor)
{
	QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);

	for (auto start = 0, end = 0; finder.toNextBoundary() != -1; start = end) {
		end = finder.position();
		auto firstCodepoint = QStringView(text).mid(start, end - start).toUcs4()[0];

		if (u_hasBinaryProperty(firstCodepoint, UCHAR_EMOJI_PRESENTATION)) {
			cursor.setPosition(start, QTextCursor::MoveAnchor);
			cursor.setPosition(end, QTextCursor::KeepAnchor);

			auto font = QGuiApplication::font();
			font.setFamily(EMOJI_FONT_FAMILY);
			font.setPointSize(font.pointSize() * emojiFontSizeFactor);

			QTextCharFormat format;
			format.setFont(font);
			cursor.setCharFormat(format);
		}
	}
}

// Marks and hightlights URLs to be displayed as links that can be opened.
static void formatUrls(QTextCursor &cursor, const QString &text)
{
	processTextParts(text, isTextSeparator, [&cursor](qsizetype i, QStringView part) {
		if (part.startsWith(URL_PREFIX)) {
			cursor.setPosition(i, QTextCursor::MoveAnchor);
			cursor.setPosition(i + part.size(), QTextCursor::KeepAnchor);

			QTextCharFormat format;

			format.setAnchor(true);
			format.setAnchorHref(part.toString());
			format.setForeground(QPalette().link());

			cursor.setCharFormat(format);
		}
	});
}

TextFormatter::TextFormatter(QObject *parent)
	: QObject(parent)
{
}

void TextFormatter::setTextDocument(QQuickTextDocument *textDocument)
{
	if (m_textDocument != textDocument) {
		m_textDocument = textDocument;
		update();
	}
}

void TextFormatter::setEnhancedFormatting(bool enhancedFormatting)
{
	if (m_enhancedFormatting != enhancedFormatting) {
		m_enhancedFormatting = enhancedFormatting;
		update();
	}
}

void TextFormatter::update()
{
	if (m_textDocument) {
		if (m_enhancedFormatting) {
			attachEnhancedTextFormatting();
		} else {
			attachTextFormatting();
		}
	}
}

void TextFormatter::attachTextFormatting()
{
	attachFormatting([](QTextCursor &cursor, const QString &text) {
		formatEmojis(cursor, text, MIXED_TEXT_EMOJI_SIZE_FACTOR);
	});
}

void TextFormatter::attachEnhancedTextFormatting()
{
	attachFormatting([](QTextCursor &cursor, const QString &text) {
		formatEmojis(cursor, text, determineEmojiFontSizeFactor(text));
		formatUrls(cursor, text);
	});
}

void TextFormatter::attachFormatting(const std::function<void (QTextCursor &cursor, const QString &text)> &formatText) {
	auto document = m_textDocument->textDocument();

	// Avoid calling this function again and creating an infinite loop if this function modifies the
	// text document.
	QObject::disconnect(document, &QTextDocument::contentsChanged, this, nullptr);

	QTextCursor cursor(document);
	auto text = document->toRawText();

	formatText(cursor, text);

	// Connect this function in order to be called each time the text document is modified.
	// That way, text changes via QML cause the formatting to be applied.
	QObject::connect(document, &QTextDocument::contentsChanged, this, [this, formatText]() {
		attachFormatting(formatText);
	});
}

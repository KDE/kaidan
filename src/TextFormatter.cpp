// SPDX-FileCopyrightText: 2021 Carson Black <uhhadd@gmail.com>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TextFormatter.h"

// Qt
#include <QGuiApplication>
#include <QPalette>
#include <QQuickTextDocument>
#include <QTextBoundaryFinder>
#include <QTextCursor>
#include <QTextDocument>
#include <unicode/uchar.h>
// Kaidan
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

static TextType determineTextType(const QString &text)
{
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

static double determineEmojiFontSizeFactor(const QString &text)
{
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

// Removes the format applied to emojis from the text in a specified range.
static void removeEmojiFormat(QTextCursor &cursor, int start, int end)
{
    cursor.setPosition(start, QTextCursor::MoveAnchor);
    cursor.setPosition(end, QTextCursor::KeepAnchor);

    auto format = cursor.charFormat();

    if (auto font = format.font(); font.family() == EMOJI_FONT_FAMILY) {
        font.setFamilies(QGuiApplication::font().families());
        font.setPointSize(QGuiApplication::font().pointSize());

        format.setFont(font);
        cursor.setCharFormat(format);
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

// Hightlights mentions of group chat users.
static void formatGroupChatUserMentions(QTextCursor &cursor, const QString &text)
{
    processTextParts(text, isTextSeparator, [&cursor](qsizetype i, QStringView part) {
        if (part.startsWith(GROUP_CHAT_USER_MENTION_PREFIX)) {
            cursor.setPosition(i, QTextCursor::MoveAnchor);
            cursor.setPosition(i + part.size(), QTextCursor::KeepAnchor);

            QTextCharFormat format;

            format.setFontWeight(QFont::DemiBold);

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
            attachEnhancedFormatting();
        } else {
            attachNormalFormatting();
        }
    }
}

void TextFormatter::attachNormalFormatting()
{
    attachFormatting([](QTextCursor &cursor, const QString &text) {
        formatEmojis(cursor, text, MIXED_TEXT_EMOJI_SIZE_FACTOR);
    });
}

void TextFormatter::attachEnhancedFormatting()
{
    attachFormatting([](QTextCursor &cursor, const QString &text) {
        formatEmojis(cursor, text, determineEmojiFontSizeFactor(text));
        formatUrls(cursor, text);
        formatGroupChatUserMentions(cursor, text);
    });
}

void TextFormatter::attachFormatting(const std::function<void(QTextCursor &cursor, const QString &text)> &formatText, int start, int addedCharactersCount)
{
    auto document = m_textDocument->textDocument();

    // Avoid calling this function again and creating an infinite loop if this function modifies the
    // text document.
    QObject::disconnect(document, &QTextDocument::contentsChange, this, nullptr);

    QTextCursor cursor(document);
    auto text = document->toRawText();

    // If text is added after an emoji, the format of the emoji is automatically applied to the added text.
    // To not display the added text with the wrong emoji format, the format of the added text is reset.
    // The added text is afterwards formatted again by formatText() which ensures that added emojis are correctly displayed as well.
    // "end <= text.size()" is needed since addedCharactersCount is sometimes out of range (for an unknown reason).
    if (const auto end = start + addedCharactersCount; addedCharactersCount > 0 && end <= text.size()) {
        removeEmojiFormat(cursor, start, end);
    }

    formatText(cursor, text);

    // Connect this function in order to be called each time the text document is modified.
    // That way, text changes via QML cause the formatting to be applied.
    QObject::connect(document, &QTextDocument::contentsChange, this, [this, formatText](int start, int, int addedCharactersCount) {
        attachFormatting(formatText, start, addedCharactersCount);
    });
}

#include "moc_TextFormatter.cpp"

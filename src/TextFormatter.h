// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class QQuickTextDocument;
class QTextCursor;

class TextFormatter : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickTextDocument *textDocument MEMBER m_textDocument WRITE setTextDocument)
    Q_PROPERTY(bool enhancedFormatting MEMBER m_enhancedFormatting WRITE setEnhancedFormatting)

public:
    TextFormatter(QObject *parent = nullptr);

    void setTextDocument(QQuickTextDocument *textDocument);
    void setEnhancedFormatting(bool enhancedFormatting);

private:
    void update();

    /**
     * Attaches formatting to a text document in order to format the text on each change with
     * minimal adjustments.
     *
     * That results in displaying the text approriately.
     * For example, emojis are correctly formatted and a bit enlarged.
     */
    void attachTextFormatting();

    /**
     * Attaches formatting to a text document in order to format the text on each change with all
     * available filters.
     *
     * That results in displaying the text approriately.
     * For example, emojis are correctly formatted and enlarged depending on their count while links
     * are marked as such and appropriately highlighted.
     */
    void attachEnhancedTextFormatting();

    void attachFormatting(const std::function<void(QTextCursor &cursor, const QString &text)> &formatText);

    QQuickTextDocument *m_textDocument = nullptr;
    bool m_enhancedFormatting = false;
};

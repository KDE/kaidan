// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LogHandler.h"

// Qt
#include <QBuffer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
// QXmpp
#include <QXmppClient.h>
// KF
#ifdef WITH_KSYNTAXHIGHLIGHTING
#include "syntax-highlighting/ansihighlighter.h"
#include <KSyntaxHighlighting/Repository>
#endif
// Kaidan
#include "KaidanCoreLog.h"

using namespace Qt::StringLiterals;
#ifdef WITH_KSYNTAXHIGHLIGHTING
using namespace KSyntaxHighlighting;
#endif

static QString applyXmlAnsiHighlighting(const QString &xml)
{
#ifdef WITH_KSYNTAXHIGHLIGHTING
    thread_local static Repository repo;
    thread_local static auto theme = repo.defaultTheme(Repository::DarkTheme);
    thread_local static auto definition = repo.definitionForName(u"xml"_s);

    AnsiHighlighter highlighter;
    highlighter.setTheme(theme);
    highlighter.setDefinition(definition);

    QString xmlAnsiHighlighted;
    highlighter.setOutputString(&xmlAnsiHighlighted);

    QByteArray bufferData = xml.toUtf8();
    QBuffer inputBuffer(&bufferData);
    inputBuffer.open(QIODevice::ReadOnly);

    highlighter.highlightData(&inputBuffer, AnsiHighlighter::AnsiFormat::XTerm256Color, AnsiHighlighter::Option::NoOptions);

    // remove '\n' at the end
    return xmlAnsiHighlighted.isEmpty() ? QString() : xmlAnsiHighlighted.chopped(1);
#else
    return xml;
#endif
}

LogHandler::LogHandler(QXmppClient *client, bool enable, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    client->logger()->setLoggingType(QXmppLogger::SignalLogging);
    enableLogging(enable);
}

void LogHandler::enableLogging(bool enabled)
{
    // check if we need to change something
    if (this->enabled == enabled)
        return;
    // update enabled status
    this->enabled = enabled;

    // apply change: enable or disable
    if (enabled)
        connect(m_client->logger(), &QXmppLogger::message, this, &LogHandler::handleLog);
    else
        disconnect(m_client->logger(), &QXmppLogger::message, this, &LogHandler::handleLog);
}

void LogHandler::handleLog(QXmppLogger::MessageType type, const QString &text)
{
    switch (type) {
    case QXmppLogger::ReceivedMessage:
        qCDebug(KAIDAN_CORE_LOG).noquote() << "[incoming] <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << Qt::endl
                                           << applyXmlAnsiHighlighting(makeXmlPretty(text));
        break;
    case QXmppLogger::SentMessage:
        qCDebug(KAIDAN_CORE_LOG).noquote() << "[outgoing] >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << Qt::endl
                                           << applyXmlAnsiHighlighting(makeXmlPretty(text));
        break;
    case QXmppLogger::WarningMessage:
        qCDebug(KAIDAN_CORE_LOG).noquote() << "[client] [warn]" << text;
        break;
    default:
        break;
    }
}

QString LogHandler::makeXmlPretty(QString xmlIn)
{
    QString xmlOut;

    QXmlStreamReader reader(xmlIn);
    QXmlStreamWriter writer(&xmlOut);
    writer.setAutoFormatting(true);

    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isWhitespace() && !reader.hasError()) {
            writer.writeCurrentToken(reader);
        }
    }

    // remove xml header
    xmlOut.replace(QStringLiteral("<?xml version=\"1.0\"?>"), QStringLiteral(""));

    // remove first & last char (\n)
    // first char is needed due to header replacement
    xmlOut = xmlOut.right(xmlOut.size() - 1);
    xmlOut = xmlOut.left(xmlOut.size() - 1);

    return xmlOut;
}

#include "moc_LogHandler.cpp"

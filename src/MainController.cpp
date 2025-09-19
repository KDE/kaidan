// SPDX-FileCopyrightText: 2016 geobra <s.g.b@gmx.de>
// SPDX-FileCopyrightText: 2016 Marzanna <MRZA-MRZA@users.noreply.github.com>
// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MainController.h"

// QXmpp
#include <QXmppUri.h>
// Kaidan
#include "AccountController.h"
#include "AccountDb.h"
#include "AvatarCache.h"
#include "Database.h"
#include "Globals.h"
#include "GroupChatUserDb.h"
#include "MessageDb.h"
#include "RosterDb.h"
#include "RosterModel.h"
#include "Settings.h"

using namespace std::chrono_literals;

MainController *MainController::s_instance = nullptr;

MainController *MainController::instance()
{
    return s_instance;
}

MainController::MainController(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    new Settings(this);

    new Database(this);
    new AccountDb(this);
    new MessageDb(this);
    new RosterDb(this);
    new GroupChatUserDb(this);

    new AccountController(this);

    new AvatarCache(this);

    m_rosterModel = new RosterModel(this);
}

MainController::~MainController()
{
    s_instance = nullptr;
}

void MainController::addOpenUri(const QString &uriString)
{
    if (const auto uriParsingResult = QXmppUri::fromString(uriString); std::holds_alternative<QXmppUri>(uriParsingResult)) {
        const auto uri = std::get<QXmppUri>(uriParsingResult);

        // Do not open XMPP URIs for group chats (e.g., "xmpp:group@groups.example.org?join") as long as Kaidan does not support that.
        if (uri.query().type() == typeid(QXmpp::Uri::Join)) {
            return;
        }

        Q_EMIT userJidReceived(uri.jid());
    }
}

void MainController::receiveMessage(const QStringList &arguments, const QString &workingDirectory)
{
    Q_UNUSED(workingDirectory)
    for (const QString &arg : arguments) {
        addOpenUri(arg);
    }
}

QString applicationProfile()
{
    static const auto profile = qEnvironmentVariable(PROFILE_VARIABLE);
    return profile;
}

#ifdef NDEBUG
QString configFileBaseName()
{
    return QStringLiteral(APPLICATION_NAME);
}

QStringList oldDatabaseFilenames()
{
    return {QStringLiteral("messages.sqlite3")};
}

QString databaseFilename()
{
    return QStringLiteral(DB_FILE_BASE_NAME ".sqlite3");
}
#else
QString applicationProfileSuffix()
{
    static const auto profileSuffix = []() -> QString {
        const auto profile = applicationProfile();
        if (!profile.isEmpty()) {
            return u'-' + profile;
        }
        return {};
    }();
    return profileSuffix;
}

QString configFileBaseName()
{
    return u"" APPLICATION_NAME % applicationProfileSuffix();
}

QStringList oldDatabaseFilenames()
{
    return {u"messages" % applicationProfileSuffix() % u".sqlite3"};
}

QString databaseFilename()
{
    return u"" DB_FILE_BASE_NAME % applicationProfileSuffix() % u".sqlite3";
}
#endif

#include "moc_MainController.cpp"

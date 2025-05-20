// SPDX-FileCopyrightText: 2016 geobra <s.g.b@gmx.de>
// SPDX-FileCopyrightText: 2016 Marzanna <MRZA-MRZA@users.noreply.github.com>
// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2020 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
#include <QStringList>

class RosterModel;

class MainController : public QObject
{
    Q_OBJECT

public:
    static MainController *instance();

    explicit MainController(QObject *parent = nullptr);
    ~MainController() override;

    /**
     * Adds an XMPP URI to be opened as soon as possible.
     */
    void addOpenUri(const QString &uriString);

    /**
     * Emitted if an XMPP URI containing a JID of a user (e.g., "xmpp:alice@example.org") is sent from the system to Kaidan.
     */
    Q_SIGNAL void userJidReceived(const QString &userJid);

    /**
     * Receives messages from another instance of the application.
     */
    Q_INVOKABLE void receiveMessage(const QStringList &arguments, const QString &workingDirectory);

    /**
     * Raises the window to the foreground so that it is on top of all other windows.
     */
    Q_SIGNAL void raiseWindowRequested();

    /**
     * Opens the start page on top of any existing pages on the lowest layer.
     */
    Q_SIGNAL void openStartPageRequested();

    /**
     * Opens the chat page for a given chat.
     *
     * @param accountJid JID of the account for that the chat page is opened
     * @param chatJid JID of the chat for that the chat page is opened
     */
    Q_SIGNAL void openChatPageRequested(const QString &accountJid, const QString &chatJid);

    /**
     * Closes the chat page.
     */
    Q_SIGNAL void closeChatPageRequested();

    /**
     * Shows a passive notification.
     */
    Q_SIGNAL void passiveNotificationRequested(QString text, const QString duration = {});

private:
    RosterModel *m_rosterModel;

    static MainController *s_instance;
};

/**
 * Returns the name of the configuration file without its file extension.
 *
 * @return the config file name without extension
 */
QString configFileBaseName();

/**
 * Returns the old names of the database file.
 *
 * @return the old database file names
 */
QStringList oldDatabaseFilenames();

/**
 * Returns the name of the database file.
 *
 * @return the database file name
 */
QString databaseFilename();

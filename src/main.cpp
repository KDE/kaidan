// SPDX-FileCopyrightText: 2016 geobra <s.g.b@gmx.de>
// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2017 Ilya Bizyaev <bizyaev@zoho.com>
// SPDX-FileCopyrightText: 2017 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include <qqml.h>

// KF6
#if __has_include("KCrash")
#include <KCrash>
#endif

// QXmpp
#include <QXmppClient.h>
#include <QXmppDiscoveryIq.h>
#include <QXmppMixInfoItem.h>
#include <QXmppMixParticipantItem.h>
#include <QXmppRegisterIq.h>
#include <QXmppResultSet.h>
#include <QXmppUri.h>
#include <QXmppVCardIq.h>
#include <QXmppVersionIq.h>

// Kaidan
#include "Account.h"
#include "AccountManager.h"
#include "AccountMigrationManager.h"
#include "AccountQrCodeGenerator.h"
#include "AtmManager.h"
#include "AuthenticatableEncryptionKeyModel.h"
#include "AuthenticatedEncryptionKeyModel.h"
#include "AvatarFileStorage.h"
#include "BitsOfBinaryImageProvider.h"
#include "Blocking.h"
#include "ChatController.h"
#include "ChatHintModel.h"
#include "ContactQrCodeGenerator.h"
#include "CredentialsGenerator.h"
#include "CredentialsValidator.h"
#include "DataFormModel.h"
#include "DiscoveryManager.h"
#include "EmojiModel.h"
#include "Encryption.h"
#include "EncryptionController.h"
#include "EncryptionWatcher.h"
#include "Enums.h"
#include "FileModel.h"
#include "FileProgressCache.h"
#include "FileProxyModel.h"
#include "FileSharingController.h"
#include "GroupChatController.h"
#include "GroupChatQrCodeGenerator.h"
#include "GroupChatUser.h"
#include "GroupChatUserFilterModel.h"
#include "GroupChatUserKeyAuthenticationFilterModel.h"
#include "GroupChatUserModel.h"
#include "GuiStyle.h"
#include "HostCompletionModel.h"
#include "HostCompletionProxyModel.h"
#include "Kaidan.h"
#include "LoginQrCodeGenerator.h"
#include "MediaUtils.h"
#include "Message.h"
#include "MessageComposition.h"
#include "MessageModel.h"
#include "MessageReactionModel.h"
#include "ProviderListModel.h"
#include "PublicGroupChatModel.h"
#include "PublicGroupChatProxyModel.h"
#include "PublicGroupChatSearchManager.h"
#include "QmlUtils.h"
#include "QrCodeScannerFilter.h"
#include "RegistrationDataFormFilterModel.h"
#include "RegistrationManager.h"
#include "RosterFilterProxyModel.h"
#include "RosterItemWatcher.h"
#include "RosterManager.h"
#include "RosterModel.h"
#include "ServerFeaturesCache.h"
#include "StatusBar.h"
#include "TextFormatter.h"
#include "UserDevicesModel.h"
#include "VCardManager.h"
#include "VCardModel.h"
#include "VersionManager.h"

Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(QMultiHash)

Q_DECLARE_METATYPE(Qt::ApplicationState)

Q_DECLARE_METATYPE(QXmppClient::State)
Q_DECLARE_METATYPE(QXmppMessage::State)
Q_DECLARE_METATYPE(QXmppDiscoveryIq)
Q_DECLARE_METATYPE(QXmppPresence)
Q_DECLARE_METATYPE(QXmppStanza::Error)
Q_DECLARE_METATYPE(QXmppResultSetReply)
Q_DECLARE_METATYPE(QXmpp::TrustLevel)
Q_DECLARE_METATYPE(QXmppUri)
Q_DECLARE_METATYPE(QXmppVCardIq)
Q_DECLARE_METATYPE(QXmppVersionIq)

Q_DECLARE_METATYPE(std::function<void()>)
Q_DECLARE_METATYPE(std::function<void(RosterItem &)>)
Q_DECLARE_METATYPE(std::function<void(Message &)>)
Q_DECLARE_METATYPE(std::function<void(GroupChatUser &)>)

Q_DECLARE_METATYPE(std::shared_ptr<Message>)

#ifndef QAPPLICATION_CLASS
#define QAPPLICATION_CLASS QApplication
#endif
#include QT_STRINGIFY(QAPPLICATION_CLASS)

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
// SingleApplication (Qt5 replacement for QtSingleApplication)
#include "singleapp/singleapplication.h"
#endif

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

enum CommandLineParseResult {
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested,
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, QString *errorMessage)
{
    // application description
    parser.setApplicationDescription(QStringLiteral(APPLICATION_DISPLAY_NAME) + QStringLiteral(" - ") + QStringLiteral(APPLICATION_DESCRIPTION));

    // add all possible arguments
    QCommandLineOption helpOption = parser.addHelpOption();
    QCommandLineOption versionOption = parser.addVersionOption();
    parser.addOption({QStringLiteral("disable-xml-log"), QStringLiteral("Disable output of full XMPP XML stream.")});
#ifndef NDEBUG
    parser.addOption({{QStringLiteral("m"), QStringLiteral("multiple")}, QStringLiteral("Allow multiple instances to be started.")});
#endif
    parser.addPositionalArgument(QStringLiteral("xmpp-uri"), QStringLiteral("An XMPP-URI to open (i.e. join a chat)."), QStringLiteral("[xmpp-uri]"));

    // parse arguments
    if (!parser.parse(QGuiApplication::arguments())) {
        *errorMessage = parser.errorText();
        return CommandLineError;
    }

    // check for special cases
    if (parser.isSet(versionOption))
        return CommandLineVersionRequested;

    if (parser.isSet(helpOption))
        return CommandLineHelpRequested;
    // if nothing special happened, return OK
    return CommandLineOk;
}

Q_DECL_EXPORT int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    //
    // App
    //

#ifdef UBUNTU_TOUCH
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "true");
    qputenv("QT_QUICK_CONTROLS_MOBILE", "true");
#endif

#ifdef APPIMAGE
    qputenv("OPENSSL_CONF", "");
#endif

    // name, display name, description
    QGuiApplication::setApplicationName(QStringLiteral(APPLICATION_NAME));
    QGuiApplication::setApplicationDisplayName(QStringLiteral(APPLICATION_DISPLAY_NAME));
    QGuiApplication::setApplicationVersion(QStringLiteral(VERSION_STRING));
    QGuiApplication::setDesktopFileName(QStringLiteral(APPLICATION_ID));

    // Set the default style for Qt Quick Controls.
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        const QString defaultStyle = QStringLiteral("Material");
#else
        const QString defaultStyle = QStringLiteral("org.kde.desktop");
#endif
        qDebug() << "QT_QUICK_CONTROLS_STYLE not set, setting to" << defaultStyle;
        qputenv("QT_QUICK_CONTROLS_STYLE", defaultStyle.toLatin1());
    }

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    QApplication::setStyle(QStringLiteral("breeze"));
#endif

    // Set the default icon theme.
    if (QIcon::fallbackThemeName().isEmpty()) {
        QIcon::setFallbackThemeName(QStringLiteral("breeze"));
    }

    // create a qt app
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    QGuiApplication app(argc, argv);
#else
    SingleApplication app(argc, argv, true);
#endif

#ifdef APPIMAGE
    QFileInfo executable(QCoreApplication::applicationFilePath());

    if (executable.isSymLink()) {
        executable.setFile(executable.symLinkTarget());
    }

    QString gstreamerPluginsPath;

    // Try to use deployed plugins if any...
#if defined(TARGET_GSTREAMER_PLUGINS)
    gstreamerPluginsPath = QString::fromLocal8Bit(TARGET_GSTREAMER_PLUGINS);

    if (!gstreamerPluginsPath.isEmpty()) {
        gstreamerPluginsPath = QDir::cleanPath(QString::fromLatin1("%1/../..%2").arg(executable.absolutePath(), gstreamerPluginsPath));
    }
    qDebug() << "Looking for gstreamer in " << gstreamerPluginsPath;
#else
    qFatal("Please provide the unified directory containing the gstreamer plugins and gst-plugin-scanner.");
#endif

#if defined(QT_DEBUG)
    qputenv("GST_DEBUG", "ERROR:5,WARNING:5,INFO:5,DEBUG:5,LOG:5");
#endif
    qputenv("GST_PLUGIN_PATH_1_0", QByteArray());
    qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", gstreamerPluginsPath.toLocal8Bit());
    qputenv("GST_PLUGIN_SCANNER_1_0", QString::fromLatin1("%1/gst-plugin-scanner").arg(gstreamerPluginsPath).toLocal8Bit());
#endif // APPIMAGE

    // register qMetaTypes
    qRegisterMetaType<ProviderListItem>();
    qRegisterMetaType<RosterItem>();
    qRegisterMetaType<RosterItemWatcher *>();
    qRegisterMetaType<RosterModel *>();
    qRegisterMetaType<RosterManager *>();
    qRegisterMetaType<Message>();
    qRegisterMetaType<MessageModel *>();
    qRegisterMetaType<ChatHintModel *>();
    qRegisterMetaType<DiscoveryManager *>();
    qRegisterMetaType<VCardManager *>();
    qRegisterMetaType<VersionManager *>();
    qRegisterMetaType<RegistrationManager *>();
    qRegisterMetaType<AccountMigrationManager *>();
    qRegisterMetaType<FileSharingController *>();
    qRegisterMetaType<EncryptionController *>();
    qRegisterMetaType<AvatarFileStorage *>();
    qRegisterMetaType<QmlUtils *>();
    qRegisterMetaType<QList<Message>>();
    qRegisterMetaType<QList<RosterItem>>();
    qRegisterMetaType<QHash<QString, RosterItem>>();
    qRegisterMetaType<std::function<void()>>();
    qRegisterMetaType<std::function<void(RosterItem &)>>();
    qRegisterMetaType<std::function<void(Message &)>>();
    qRegisterMetaType<QXmppVCardIq>();
    qRegisterMetaType<QMimeType>();
    qRegisterMetaType<CredentialsValidator *>();
    qRegisterMetaType<QXmppVersionIq>();
    qRegisterMetaType<QXmppUri>();
    qRegisterMetaType<QMap<QString, QUrl>>();
    qRegisterMetaType<std::shared_ptr<Message>>();
    qRegisterMetaType<AtmManager *>();
    qRegisterMetaType<GroupChatController *>();
    qRegisterMetaType<GroupChatUser>();
    qRegisterMetaType<QList<GroupChatUser>>();
    qRegisterMetaType<std::function<void(GroupChatUser &)>>();
    qRegisterMetaType<GroupChatUserModel *>();
    // The alias is needed because the type shares its QMetaType::id() with quint32.
    qRegisterMetaType<uint32_t>("uint32_t");

    // Enums for c++ member calls using enums
    qRegisterMetaType<Qt::ApplicationState>();
    qRegisterMetaType<QXmppClient::State>();
    qRegisterMetaType<QXmppMessage::State>();
    qRegisterMetaType<QXmppStanza::Error>();
    qRegisterMetaType<MessageType>();
    qRegisterMetaType<Enums::ConnectionState>();
    qRegisterMetaType<PublicGroupChatModel::CustomRole>();
    qRegisterMetaType<ClientWorker::ConnectionError>();
    qRegisterMetaType<Enums::MessageType>();
    qRegisterMetaType<Presence::Availability>();
    qRegisterMetaType<Enums::DeliveryState>();
    qRegisterMetaType<MessageOrigin>();
    qRegisterMetaType<ProviderListModel::Role>();
    qRegisterMetaType<ChatState::State>();
    qRegisterMetaType<Encryption>();
    qRegisterMetaType<MessageReactionDeliveryState>();
    qRegisterMetaType<FileModel::Role>();
    qRegisterMetaType<FileProxyModel::Mode>();
    qRegisterMetaType<Account::ContactNotificationRule>();
    qRegisterMetaType<Account::GroupChatNotificationRule>();
    qRegisterMetaType<Account::AutomaticMediaDownloadsRule>();
    qRegisterMetaType<Account::GeoLocationMapService>();
    qRegisterMetaType<RosterItem::AutomaticMediaDownloadsRule>();
    qRegisterMetaType<AccountMigrationManager::MigrationState>();
    qRegisterMetaType<RosterItem::GroupChatFlag>();
    qRegisterMetaType<RosterModel::RosterItemRoles>();
    qRegisterMetaType<Message::TrustLevel>();

    // QXmpp
    qRegisterMetaType<QXmppResultSetReply>();
    qRegisterMetaType<QXmppMessage>();
    qRegisterMetaType<QXmppPresence>();
    qRegisterMetaType<QXmppDiscoveryIq>();
    qRegisterMetaType<QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>>>();
    qRegisterMetaType<QXmppMixInfoItem>();
    qRegisterMetaType<QXmppMixParticipantItem>();
    qRegisterMetaType<QMultiHash<QString, QByteArray>>();

    // Qt-Translator
    QTranslator qtTranslator;
    qtTranslator.load(QStringLiteral("qt_") + QLocale::system().name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath));
    QCoreApplication::installTranslator(&qtTranslator);

    //
    // Command line arguments
    //

    // create parser and add a description
    QCommandLineParser parser;
    // parse the arguments
    QString commandLineErrorMessage;
    switch (parseCommandLine(parser, &commandLineErrorMessage)) {
    case CommandLineError:
        qWarning() << commandLineErrorMessage;
        return 1;
    case CommandLineVersionRequested:
        parser.showVersion();
        return 0;
    case CommandLineHelpRequested:
        parser.showHelp();
        return 0;
    case CommandLineOk:
        break;
    }

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
#ifdef NDEBUG
    if (app.isSecondary()) {
        qDebug() << "Another instance of" << APPLICATION_DISPLAY_NAME << "is already running!";
#else
    // check if another instance already runs
    if (app.isSecondary() && !parser.isSet(QStringLiteral("multiple"))) {
        qDebug().noquote() << QStringLiteral("Another instance of %1 is already running.").arg(QStringLiteral(APPLICATION_DISPLAY_NAME))
                           << "You can enable multiple instances by specifying '--multiple'.";
#endif

        // send a possible link to the primary instance
        if (const auto positionalArguments = parser.positionalArguments(); !positionalArguments.isEmpty())
            app.sendMessage(positionalArguments.first().toUtf8());
        return 0;
    }
#endif

    //
    // Kaidan back-end
    //
    Kaidan kaidan(!parser.isSet(QStringLiteral("disable-xml-log")));

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    // receive messages from other instances of Kaidan
    Kaidan::connect(&app, &SingleApplication::receivedMessage, &kaidan, &Kaidan::receiveMessage);
#endif

    // open the XMPP-URI/link (if given)
    if (const auto positionalArguments = parser.positionalArguments(); !positionalArguments.isEmpty())
        kaidan.addOpenUri(positionalArguments.first());

    //
    // QML-GUI
    //

    QQmlApplicationEngine engine;

    engine.addImageProvider(QLatin1String(BITS_OF_BINARY_IMAGE_PROVIDER_NAME), BitsOfBinaryImageProvider::instance());

#if __has_include("KCrash")
    KCrash::initialize();
#endif

    // QML type bindings
    qmlRegisterType<AccountQrCodeGenerator>(APPLICATION_ID, 1, 0, "AccountQrCodeGenerator");
    qmlRegisterType<StatusBar>(APPLICATION_ID, 1, 0, "StatusBar");
    qmlRegisterType<EmojiModel>(APPLICATION_ID, 1, 0, "EmojiModel");
    qmlRegisterType<EmojiProxyModel>(APPLICATION_ID, 1, 0, "EmojiProxyModel");
#ifdef NEED_TO_PORT
    qmlRegisterType<QrCodeScannerFilter>(APPLICATION_ID, 1, 0, "QrCodeScannerFilter");
#endif
    qmlRegisterType<VCardModel>(APPLICATION_ID, 1, 0, "VCardModel");
    qmlRegisterType<RosterFilterProxyModel>(APPLICATION_ID, 1, 0, "RosterFilterProxyModel");
    qmlRegisterType<BlockingModel>(APPLICATION_ID, 1, 0, "BlockingModel");
    qmlRegisterType<BlockingWatcher>(APPLICATION_ID, 1, 0, "BlockingWatcher");
    qmlRegisterType<BlockingAction>(APPLICATION_ID, 1, 0, "BlockingAction");
    qmlRegisterType<ContactQrCodeGenerator>(APPLICATION_ID, 1, 0, "ContactQrCodeGenerator");
    qmlRegisterType<MessageComposition>(APPLICATION_ID, 1, 0, "MessageComposition");
    qmlRegisterType<FileSelectionModel>(APPLICATION_ID, 1, 0, "FileSelectionModel");
    qmlRegisterType<LoginQrCodeGenerator>(APPLICATION_ID, 1, 0, "LoginQrCodeGenerator");
    qmlRegisterType<UserDevicesModel>(APPLICATION_ID, 1, 0, "UserDevicesModel");
    qmlRegisterType<CredentialsGenerator>(APPLICATION_ID, 1, 0, "CredentialsGenerator");
    qmlRegisterType<CredentialsValidator>(APPLICATION_ID, 1, 0, "CredentialsValidator");
    qmlRegisterType<RegistrationDataFormFilterModel>(APPLICATION_ID, 1, 0, "RegistrationDataFormFilterModel");
    qmlRegisterType<ProviderListModel>(APPLICATION_ID, 1, 0, "ProviderListModel");
    qmlRegisterType<FileProgressWatcher>(APPLICATION_ID, 1, 0, "FileProgressWatcher");
    qmlRegisterType<UserPresenceWatcher>(APPLICATION_ID, 1, 0, "UserPresenceWatcher");
    qmlRegisterType<UserResourcesWatcher>(APPLICATION_ID, 1, 0, "UserResourcesWatcher");
    qmlRegisterType<RosterItemWatcher>(APPLICATION_ID, 1, 0, "RosterItemWatcher");
    qmlRegisterType<PublicGroupChatSearchManager>(APPLICATION_ID, 1, 0, "PublicGroupChatSearchManager");
    qmlRegisterType<PublicGroupChatModel>(APPLICATION_ID, 1, 0, "PublicGroupChatModel");
    qmlRegisterType<PublicGroupChatProxyModel>(APPLICATION_ID, 1, 0, "PublicGroupChatProxyModel");
    qmlRegisterType<AuthenticatableEncryptionKeyModel>(APPLICATION_ID, 1, 0, "AuthenticatableEncryptionKeyModel");
    qmlRegisterType<AuthenticatedEncryptionKeyModel>(APPLICATION_ID, 1, 0, "AuthenticatedEncryptionKeyModel");
    qmlRegisterType<EncryptionWatcher>(APPLICATION_ID, 1, 0, "EncryptionWatcher");
    qmlRegisterType<HostCompletionModel>(APPLICATION_ID, 1, 0, "HostCompletionModel");
    qmlRegisterType<HostCompletionProxyModel>(APPLICATION_ID, 1, 0, "HostCompletionProxyModel");
    qmlRegisterType<FileModel>(APPLICATION_ID, 1, 0, "FileModel");
    qmlRegisterType<FileProxyModel>(APPLICATION_ID, 1, 0, "FileProxyModel");
    qmlRegisterType<TextFormatter>(APPLICATION_ID, 1, 0, "TextFormatter");
    qmlRegisterType<GroupChatQrCodeGenerator>(APPLICATION_ID, 1, 0, "GroupChatQrCodeGenerator");
    qmlRegisterType<GroupChatUserModel>(APPLICATION_ID, 1, 0, "GroupChatUserModel");
    qmlRegisterType<GroupChatUserFilterModel>(APPLICATION_ID, 1, 0, "GroupChatUserFilterModel");
    qmlRegisterType<GroupChatUserKeyAuthenticationFilterModel>(APPLICATION_ID, 1, 0, "GroupChatUserKeyAuthenticationFilterModel");
    qmlRegisterType<AccountTrustMessageUriGenerator>(APPLICATION_ID, 1, 0, "AccountTrustMessageUriGenerator");
    qmlRegisterType<ContactTrustMessageUriGenerator>(APPLICATION_ID, 1, 0, "ContactTrustMessageUriGenerator");
    qmlRegisterType<MessageReactionModel>(APPLICATION_ID, 1, 0, "MessageReactionModel");

    qmlRegisterUncreatableType<Account>(APPLICATION_ID, 1, 0, "Account", QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<QAbstractItemModel>(APPLICATION_ID, 1, 0, "QAbstractItemModel", QStringLiteral("Used by proxy models"));
    qmlRegisterUncreatableType<Emoji>(APPLICATION_ID, 1, 0, "Emoji", QStringLiteral("Used by emoji models"));
    qmlRegisterUncreatableType<QMimeType>(APPLICATION_ID, 1, 0, "QMimeType", QStringLiteral("QMimeType type usable"));
    qmlRegisterUncreatableType<ClientWorker>(APPLICATION_ID, 1, 0, "ClientWorker", QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<DataFormModel>(APPLICATION_ID, 1, 0, "DataFormModel", QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<Presence>(APPLICATION_ID, 1, 0, "Presence", QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<RegistrationManager>(APPLICATION_ID, 1, 0, "RegistrationManager", QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<AccountMigrationManager>(APPLICATION_ID,
                                                        1,
                                                        0,
                                                        "AccountMigrationManager",
                                                        QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<ChatState>(APPLICATION_ID, 1, 0, "ChatState", QStringLiteral("Cannot create object; only enums defined"));
    qmlRegisterUncreatableType<RosterModel>(APPLICATION_ID, 1, 0, "RosterModel", QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<ServerFeaturesCache>(APPLICATION_ID, 1, 0, "ServerFeaturesCache", QStringLiteral("ServerFeaturesCache type usable"));
    qmlRegisterUncreatableType<Encryption>(APPLICATION_ID, 1, 0, "Encryption", QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<File>(APPLICATION_ID, 1, 0, "File", QStringLiteral("Not creatable from QML"));
    qmlRegisterUncreatableType<PublicGroupChat>(APPLICATION_ID, 1, 0, "PublicGroupChat", QStringLiteral("Used by PublicGroupChatModel"));
    qmlRegisterUncreatableType<HostCompletionModel>(APPLICATION_ID, 1, 0, "HostCompletionModel", QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<MessageReactionDeliveryState>(APPLICATION_ID,
                                                             1,
                                                             0,
                                                             "MessageReactionDeliveryState",
                                                             QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<RosterItem>(APPLICATION_ID, 1, 0, "RosterItem", QStringLiteral("Cannot create object; only enums defined!"));
    qmlRegisterUncreatableType<Message>(APPLICATION_ID, 1, 0, "Message", QStringLiteral("Cannot create object; only enums defined!"));

    qmlRegisterUncreatableMetaObject(ChatState::staticMetaObject,
                                     APPLICATION_ID,
                                     1,
                                     0,
                                     "ChatState",
                                     QStringLiteral("Can't create object; only enums defined!"));
    qmlRegisterUncreatableMetaObject(Enums::staticMetaObject, APPLICATION_ID, 1, 0, "Enums", QStringLiteral("Can't create object; only enums defined!"));

    qmlRegisterSingletonType<QmlUtils>(APPLICATION_ID, 1, 0, "Utils", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(QmlUtils::instance());
    });
    qmlRegisterSingletonType<MediaUtils>(APPLICATION_ID, 1, 0, "MediaUtils", [](QQmlEngine *, QJSEngine *) {
        QObject *instance = new MediaUtils(qApp);
        return instance;
    });
    qmlRegisterSingletonType<Kaidan>(APPLICATION_ID, 1, 0, "Kaidan", [](QQmlEngine *engine, QJSEngine *) {
        engine->setObjectOwnership(Kaidan::instance(), QQmlEngine::CppOwnership);
        return static_cast<QObject *>(Kaidan::instance());
    });
    qmlRegisterSingletonType<GuiStyle>(APPLICATION_ID, 1, 0, "Style", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(new GuiStyle(QCoreApplication::instance()));
    });
    qmlRegisterSingletonType<AccountManager>(APPLICATION_ID, 1, 0, "AccountManager", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(AccountManager::instance());
    });
    qmlRegisterSingletonType<ChatController>(APPLICATION_ID, 1, 0, "ChatController", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(ChatController::instance());
    });
    qmlRegisterSingletonType<GroupChatController>(APPLICATION_ID, 1, 0, "GroupChatController", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(GroupChatController::instance());
    });
    qmlRegisterSingletonType<EncryptionController>(APPLICATION_ID, 1, 0, "EncryptionController", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(EncryptionController::instance());
    });
    qmlRegisterSingletonType<RosterModel>(APPLICATION_ID, 1, 0, "RosterModel", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(RosterModel::instance());
    });
    qmlRegisterSingletonType<MessageModel>(APPLICATION_ID, 1, 0, "MessageModel", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(MessageModel::instance());
    });
    qmlRegisterSingletonType<ChatHintModel>(APPLICATION_ID, 1, 0, "ChatHintModel", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(ChatHintModel::instance());
    });
    qmlRegisterSingletonType<HostCompletionModel>(APPLICATION_ID, 1, 0, "HostCompletionModel", [](QQmlEngine *, QJSEngine *) {
        static auto self = new HostCompletionModel(qApp);
        return static_cast<QObject *>(self);
    });

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

#ifdef Q_OS_ANDROID
    QtAndroid::hideSplashScreen();
#endif

    // enter qt main loop
    return app.exec();
}

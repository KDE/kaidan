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
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLibraryInfo>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickStyle>
#include <QRegularExpression>
#include <QTranslator>
#include <QWindow>
#include <QtLogging>
#include <qqml.h>
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#endif
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_HAIKU)
#include <private/qtx11extras_p.h>
#endif
// Windows
#ifdef Q_OS_WIN
#include <windows.h>
#endif
// KDE
#if __has_include("KCrash")
#include <KCrash>
#endif
#if __has_include("KWindowSystem")
#include <KWindowSystem>
#endif
#if defined(Q_OS_WINDOWS)
#include <KIconTheme>
#endif
// KDAB
#include <kdsingleapplication.h>
// GStreamer
#include <gst/gst.h>
// Kaidan
#include "Account.h"
#include "AccountMigrationController.h"
#include "Globals.h"
#include "GlobalsGen.h"
#include "ImageProvider.h"
#include "KaidanLog.h"
#include "Keychain.h"
#include "MainController.h"
#include "RosterModel.h"

Q_IMPORT_QML_PLUGIN(KaidanQmlPlugin)

const auto QUICK_CONTROLS_STYLE_VARIABLE = "QT_QUICK_CONTROLS_STYLE";
const auto QUICK_CONTROLS_DEFAULT_DESKTOP_STYLE = QStringLiteral("org.kde.desktop");
const auto QUICK_CONTROLS_DEFAULT_MOBILE_STYLE = QStringLiteral("Material");

// Environment variable containing strings used to filter out log messages that contain them.
// The strings must be separated by semicolons.
const auto KAIDAN_LOG_FILTER_VARIABLE = "KAIDAN_LOG_FILTER";
constexpr auto KAIDAN_LOG_FILTER_SEPARATOR = QLatin1Char(';');
// Strings used to always filter out log messages containing them.
const QStringList KAIDAN_DEFAULT_FILTER_STRINGS = {
    QStringLiteral("Previously registered enum will be overwritten due to name clash"),
    QStringLiteral("Possible conflicting items"),
    QStringLiteral("from scope * injected by *"),
};
QtMessageHandler originalLogMessageHandler = nullptr;

enum CommandLineParseResult {
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested,
    CommandLineUnencryptedKeychainRequested,
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, QString *errorMessage)
{
    // application description
    parser.setApplicationDescription(QStringLiteral(APPLICATION_DISPLAY_NAME) + QStringLiteral(" - ") + QStringLiteral(APPLICATION_DESCRIPTION));

    // add all possible arguments
    QCommandLineOption helpOption = parser.addHelpOption();
    QCommandLineOption versionOption = parser.addVersionOption();
    QCommandLineOption unencryptedKeychainOption = {{QStringLiteral("u"), QStringLiteral("unencrypted-keychain")},
                                                    QStringLiteral("Store passwords in an unencrypted file.")};
#ifndef NDEBUG
    parser.addOption({{QStringLiteral("m"), QStringLiteral("multiple")}, QStringLiteral("Allow multiple instances to be started.")});
#endif
    parser.addOption(unencryptedKeychainOption);
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

    if (parser.isSet(unencryptedKeychainOption))
        return CommandLineUnencryptedKeychainRequested;

    // if nothing special happened, return OK
    return CommandLineOk;
}

void filterLog(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    using namespace Qt::Literals::StringLiterals;
    static QAtomicInt initialized;
    static QMutex mutex;
    static QStringList filterStrings;

    if (initialized == 0) {
        QMutexLocker locker(&mutex);

        if (initialized == 0) {
            filterStrings = qEnvironmentVariable(KAIDAN_LOG_FILTER_VARIABLE).split(KAIDAN_LOG_FILTER_SEPARATOR, Qt::SkipEmptyParts);
            filterStrings.append(KAIDAN_DEFAULT_FILTER_STRINGS);

            initialized = 1;
        }
    }

    if (const auto match =
            std::ranges::find_if(filterStrings,
                                 [&msg](const QString &filter) {
                                     const auto rx =
                                         QRegularExpression::fromWildcard(filter, Qt::CaseInsensitive, QRegularExpression::UnanchoredWildcardConversion);
                                     return msg.contains(rx);
                                 });
        match != filterStrings.cend()) {
        return;
    }

    if (originalLogMessageHandler) {
        (*originalLogMessageHandler)(type, context, msg);
    }
}

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    originalLogMessageHandler = qInstallMessageHandler(filterLog);

#ifdef Q_OS_WIN
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    //
    // App
    //

    // Initialize the resources from Kaidan's core library.
    Q_INIT_RESOURCE(data);
    Q_INIT_RESOURCE(misc);
#if defined(Q_OS_ANDROID)
    Q_INIT_RESOURCE(notifications);
#endif
#if defined(Q_OS_ANDROID) || defined(Q_OS_WINDOWS) || defined(Q_OS_APPLE)
    Q_INIT_RESOURCE(images);
#endif

#if defined(Q_OS_WINDOWS)
    KIconTheme::initTheme();
#endif

    // name, display name, description
    QGuiApplication::setApplicationName(QStringLiteral(APPLICATION_NAME));
    QGuiApplication::setApplicationDisplayName(QStringLiteral(APPLICATION_DISPLAY_NAME));
    QGuiApplication::setApplicationVersion(QStringLiteral(VERSION_STRING));
    QGuiApplication::setDesktopFileName(QStringLiteral(APPLICATION_ID));

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // Set the window icon for X11.
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral(APPLICATION_NAME)));
#endif

    // Set the Qt Quick Controls style explicitly.
    // That is needed since setting the environment variable while using specific platform themes
    // (e.g., QT_QPA_PLATFORMTHEME=xdgdesktopportal) is not sufficient for unclear reasons.
    if (qEnvironmentVariableIsEmpty(QUICK_CONTROLS_STYLE_VARIABLE)) {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        const QString defaultStyle = QUICK_CONTROLS_DEFAULT_MOBILE_STYLE;
#else
        const QString defaultStyle = QUICK_CONTROLS_DEFAULT_DESKTOP_STYLE;
#endif
        qCDebug(KAIDAN_LOG) << QUICK_CONTROLS_STYLE_VARIABLE << "not set, using" << defaultStyle;
        QQuickStyle::setStyle(defaultStyle);
    } else {
        QQuickStyle::setStyle(QString::fromLatin1(qgetenv(QUICK_CONTROLS_STYLE_VARIABLE)));
    }

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    QApplication::setStyle(QStringLiteral("breeze"));
#endif

    // Set the default icon theme.
    if (QIcon::fallbackThemeName().isEmpty()) {
        QIcon::setFallbackThemeName(QStringLiteral("breeze"));
    }

    QApplication app(argc, argv);

    // Keychain

    if (!QKeychain::isAvailable()) {
        qCWarning(KAIDAN_LOG, "Account passwords will be stored in unencrypted file since no password manager is available");
        QKeychainFuture::setUnencryptedFallback(true);
    }

    //
    // Command line arguments
    //

    // create parser and add a description
    QCommandLineParser parser;
    // parse the arguments
    QString commandLineErrorMessage;
    switch (parseCommandLine(parser, &commandLineErrorMessage)) {
    case CommandLineError:
        qCWarning(KAIDAN_LOG) << commandLineErrorMessage;
        return 1;
    case CommandLineVersionRequested:
        parser.showVersion();
        return 0;
    case CommandLineHelpRequested:
        parser.showHelp();
        return 0;
    case CommandLineUnencryptedKeychainRequested:
        qCWarning(KAIDAN_LOG, "Passwords will be stored in an unencrypted file");
        QKeychainFuture::setUnencryptedFallback(true);
        Q_FALLTHROUGH();
    case CommandLineOk:
        break;
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // Create a single-instance policy application.
    KDSingleApplication sapp;

    auto generateSingleApplicationMessage = [&app] {
        QJsonDocument doc;

        QJsonObject obj;
        obj[QLatin1String("working_dir")] = QDir::currentPath();
        obj[QLatin1String("args")] = QJsonArray::fromStringList(app.arguments());
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_HAIKU)
        if (KWindowSystem::isPlatformWayland()) {
            obj[QLatin1String("xdg_activation_token")] = qEnvironmentVariable("XDG_ACTIVATION_TOKEN");
        } else if (KWindowSystem::isPlatformX11()) {
            obj[QLatin1String("startup_id")] = QString::fromUtf8(QX11Info::nextStartupId());
        }
#endif // !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_HAIKU)

        doc.setObject(obj);

        return doc.toJson(QJsonDocument::Compact);
    };

    if (!sapp.isPrimaryInstance()) {
#ifdef NDEBUG
        qCDebug(KAIDAN_LOG) << "Another instance of" << APPLICATION_DISPLAY_NAME << "is already running!";
        sapp.sendMessage(generateSingleApplicationMessage());
        return 0;
#else
        // check if another instance already runs
        if (!parser.isSet(QStringLiteral("multiple"))) {
            qCDebug(KAIDAN_LOG).noquote() << QStringLiteral("Another instance of %1 is already running.").arg(QStringLiteral(APPLICATION_DISPLAY_NAME))
                                          << "You can enable multiple instances by specifying '--multiple'.";
            sapp.sendMessage(generateSingleApplicationMessage());
            return 0;
        }
#endif // NDEBUG
    }

#endif // !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)

    MainController mainController(nullptr);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // receive messages from other instances of Kaidan
    QApplication::connect(&sapp, &KDSingleApplication::messageReceived, &app, [&mainController](const QByteArray &messageData) {
        QJsonDocument doc = QJsonDocument::fromJson(messageData);
        QJsonObject message = doc.object();

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_HAIKU)
        if (KWindowSystem::isPlatformWayland()) {
            qputenv("XDG_ACTIVATION_TOKEN", message[QLatin1String("xdg_activation_token")].toString().toUtf8());
        } else if (KWindowSystem::isPlatformX11()) {
            QX11Info::setNextStartupId(message[QLatin1String("startup_id")].toString().toUtf8());
        }
#endif
        QStringList arguments;

        const auto argumentsJson = message[QLatin1String("args")].toArray();
        for (const QJsonValue &val : argumentsJson) {
            arguments << val.toString();
        }

        mainController.receiveMessage(arguments, message[QLatin1String("working_dir")].toString());
    });
#endif // !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)

    // open the XMPP-URI/link (if given)
    if (const auto positionalArguments = parser.positionalArguments(); !positionalArguments.isEmpty())
        mainController.addOpenUri(positionalArguments.first());

    //
    // QML-GUI
    //

    QQmlApplicationEngine engine;

    new ImageProvider(&engine);

#if __has_include("KCrash")
    if (QStringLiteral(BUILD_TYPE).compare(u"release", Qt::CaseInsensitive) == 0) {
        KCrash::initialize();
    }
#endif

    // If the GStreamer plugins are shipped next to the application (e.g., inside the macOS .app
    // bundle), point GStreamer at them. TARGET_GSTREAMER_PLUGINS is empty otherwise, so GStreamer
    // keeps using its default (system) plugin search path.
    if (const QString gstreamerPlugins = QStringLiteral(TARGET_GSTREAMER_PLUGINS); !gstreamerPlugins.isEmpty()) {
        const QString path = QDir::isAbsolutePath(gstreamerPlugins)
            ? gstreamerPlugins
            : QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1Char('/') + gstreamerPlugins);
        qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", QFile::encodeName(path));
    }

    // Allow importing org.freedesktop.gstreamer.Qt6GLVideoItem and using GstGLQt6VideoItem in QML.
    gst_init(&argc, &argv);
    gst_element_factory_make("qml6glsink", NULL);

    engine.loadFromModule(APPLICATION_ID, u"Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

#ifdef Q_OS_ANDROID
    QtAndroid::hideSplashScreen();
#endif

    // enter qt main loop
    auto returnCode = app.exec();

    gst_deinit();

    return returnCode;
}

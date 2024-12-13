// SPDX-FileCopyrightText: 2024 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "QXmppFileMetadata.h"
#include "QXmppHttpFileSource.h"
#include "QXmppMessage.h"
#include <QCoreApplication>
#include <QXmppClient.h>
#include <QXmppTask.h>
#include <QXmppUtils.h>

#if QXMPP_VERSION >= QT_VERSION_CHECK(1, 7, 0)
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QXmppConfiguration config;
    config.setIgnoreSslErrors(true);
    config.setJid(qEnvironmentVariable("JID"));
    config.setPassword(qEnvironmentVariable("PASSWORD"));

    QXmppClient client;
    client.logger()->setLoggingType(QXmppLogger::StdoutLogging);
    client.connectToServer(config);

    QObject::connect(&client, &QXmppClient::connected, &app, [&]() {
        QXmppMessage message;
        message.setId(QXmppUtils::generateStanzaUuid());
        message.setTo(qEnvironmentVariable("JID"));
        message.setBody(QStringLiteral("a screenshot i took"));
        QXmppFileMetadata metadata;
        metadata.setFilename(QStringLiteral("screenshot.png"));
        metadata.setDescription(QStringLiteral("Screenshot"));
        QXmppFileShare fs;
        fs.setMetadata(metadata);
        fs.setId(QStringLiteral("file1"));

        message.setSharedFiles({fs});

        client.send(std::move(message));

        QXmppHttpFileSource source;
        source.setUrl(QUrl(QStringLiteral("https://www.kaidan.im/images/screenshot.png")));

        QXmppFileSourcesAttachment sourceAttachment;
        sourceAttachment.setHttpSources({source});
        sourceAttachment.setId(QStringLiteral("file1"));

        QXmppMessage attachSource;
        attachSource.setTo(qEnvironmentVariable("JID"));
        attachSource.setAttachId(message.id());
        attachSource.setFileSourcesAttachments({sourceAttachment});

        client.send(std::move(attachSource));

        client.disconnectFromServer();
        QCoreApplication::exit();
    });

    return app.exec();
}
#else
int main()
{
    qDebug() << "Requires Kaidan to be built with QXmpp >= 1.7";
    return 1;
}
#endif

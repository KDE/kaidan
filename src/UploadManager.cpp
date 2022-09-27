/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "UploadManager.h"

// Qt
#include <QDateTime>
#include <QFileInfo>
// QXmpp
#include <QXmppUtils.h>
#include "qxmpp-exts/QXmppUploadManager.h"
// Kaidan
#include "FileProgressCache.h"
#include "Kaidan.h"
#include "MediaUtils.h"
#include "MessageDb.h"
#include "RosterManager.h"
#include "ServerFeaturesCache.h"

UploadManager::UploadManager(QXmppClient *client, RosterManager* rosterManager,
                             QObject* parent)
	: QObject(parent),
	  m_client(client),
	  m_manager(new QXmppUploadManager),
	  m_rosterManager(rosterManager)
{
	client->addExtension(m_manager);

	connect(this, &UploadManager::sendFileRequested, this, &UploadManager::sendFile);

	connect(m_manager, &QXmppUploadManager::serviceFoundChanged, this, [this] {
		Kaidan::instance()->serverFeaturesCache()->setHttpUploadSupported(m_manager->serviceFound());
	});
	connect(m_manager, &QXmppUploadManager::uploadSucceeded,
	        this, &UploadManager::handleUploadSucceeded);
	connect(m_manager, &QXmppUploadManager::uploadFailed,
	        this, &UploadManager::handleUploadFailed);
}

void UploadManager::sendFile(const QString &jid, const QUrl &fileUrl, const QString &body)
{
	// TODO: Add offline media message cache and send when connnected again
	if (m_client->state() != QXmppClient::ConnectedState) {
		emit Kaidan::instance()->passiveNotificationRequested(
			tr("Could not send file, as a result of not being connected.")
		);
		qWarning() << "[client] [UploadManager] Could not send file, as a result of "
		              "not being connected.";
		return;
	}

	qDebug() << "[client] [UploadManager] Adding upload for file:" << fileUrl;

	// toString() is used for android's content:/image:-URLs
	QFileInfo file(fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString());
	const QXmppHttpUpload* upload = m_manager->uploadFile(file);
	const QMimeType mimeType = MediaUtils::mimeType(fileUrl);
	const MessageType messageType = MediaUtils::messageType(mimeType);
	const QString msgId = QXmppUtils::generateStanzaUuid();

	Message msg;
	msg.from = m_client->configuration().jidBare();
	msg.to = jid;
	msg.id = msgId;
	msg.isOwn = true;
	msg.body = body;
	msg.stamp = QDateTime::currentDateTimeUtc();
	msg.mediaLocation = file.filePath();

	// cache message and upload
	FileProgressCache::instance()
		.reportProgress(msgId, FileProgress { 0, quint64(upload->bytesTotal()), 0.0F });
	MessageDb::instance()->addMessage(msg, MessageOrigin::UserInput);
	m_messages[upload->id()] = std::move(msg);

	connect(upload, &QXmppHttpUpload::bytesSentChanged, this, [upload, msgId] {
		auto sent = quint64(upload->bytesSent());
		auto total = quint64(upload->bytesTotal());
		FileProgressCache::instance()
			.reportProgress(msgId, FileProgress { sent, total, float(sent) / float(total) });
	});
}

void UploadManager::handleUploadSucceeded(const QXmppHttpUpload *upload)
{
	qDebug() << "[client] [UploadManager] A file upload has succeeded. Now sending message.";

	auto &originalMsg = m_messages[upload->id()];

	const QString oobUrl = upload->slot().getUrl().toEncoded();
	const QString body = originalMsg.body.isEmpty()
	                     ? oobUrl
	                     : originalMsg.body + "\n" + oobUrl;

	MessageDb::instance()->updateMessage(originalMsg.id, [oobUrl](Message &msg) {
		msg.outOfBandUrl = oobUrl;
	});

	// send message
	QXmppMessage m(originalMsg.from, originalMsg.to, body);
	m.setId(originalMsg.id);
	m.setReceiptRequested(true);
	m.setStamp(originalMsg.stamp);
	m.setOutOfBandUrl(upload->slot().getUrl().toEncoded());

	bool success = m_client->sendPacket(m);
	if (success) {
		MessageDb::instance()->updateMessage(originalMsg.id, [](Message &msg) {
			msg.deliveryState = Enums::DeliveryState::Sent;
			msg.errorText.clear();
		});
	} else {
		emit Kaidan::instance()->passiveNotificationRequested(tr("Message could not be sent."));
		MessageDb::instance()->updateMessage(originalMsg.id, [](Message &msg) {
			msg.deliveryState = Enums::DeliveryState::Error;
			msg.errorText = QStringLiteral("Message could not be sent.");
		});
	}

	FileProgressCache::instance().reportProgress(originalMsg.id, std::nullopt);
	m_messages.erase(upload->id());
}

void UploadManager::handleUploadFailed(const QXmppHttpUpload *upload)
{
	qDebug() << "[client] [UploadManager] A file upload has failed.";
	const QString &msgId = m_messages[upload->id()].id;
	m_messages.erase(upload->id());
	FileProgressCache::instance().reportProgress(msgId, std::nullopt);
}

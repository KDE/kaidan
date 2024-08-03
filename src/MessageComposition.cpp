// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageComposition.h"

// Kaidan
#include "Algorithms.h"
#include "ChatController.h"
#include "FileSharingController.h"
#include "Kaidan.h"
#include "MediaUtils.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "QmlUtils.h"
#include "RosterModel.h"
#include "ServerFeaturesCache.h"
// Qt
#include <QFileDialog>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QMimeDatabase>
// QXmpp
#include <QXmppUtils.h>
// KF
#include <KIO/PreviewJob>
#include <KFileItem>

constexpr auto THUMBNAIL_SIZE = 200;

MessageComposition::MessageComposition()
	: m_fileSelectionModel(new FileSelectionModel(this))
{
	connect(qGuiApp, &QGuiApplication::aboutToQuit, this, [this]() {
		saveDraft();
	});
}

void MessageComposition::setAccountJid(const QString &accountJid)
{
	if (m_accountJid != accountJid) {
		// Save the draft of the last chat when the current chat is changed.
		saveDraft();

		m_accountJid = accountJid;
		Q_EMIT accountJidChanged();

		loadDraft();
	}
}

void MessageComposition::setChatJid(const QString &chatJid)
{
	if (m_chatJid != chatJid) {
		// Save the draft of the last chat when the current chat is changed.
		saveDraft();

		m_chatJid = chatJid;
		Q_EMIT chatJidChanged();

		loadDraft();
	}
}

void MessageComposition::setReplaceId(const QString &replaceId)
{
	if (m_replaceId != replaceId) {
		m_replaceId = replaceId;
		Q_EMIT replaceIdChanged();
	}
}

void MessageComposition::setBody(const QString &body)
{
	if (m_body != body) {
		m_body = body;
		Q_EMIT bodyChanged();
	}
}

void MessageComposition::setSpoiler(bool spoiler)
{
	if (m_spoiler != spoiler) {
		m_spoiler = spoiler;
		Q_EMIT isSpoilerChanged();
	}
}

void MessageComposition::setSpoilerHint(const QString &spoilerHint)
{
	if (m_spoilerHint != spoilerHint) {
		m_spoilerHint = spoilerHint;
		Q_EMIT spoilerHintChanged();
	}
}

void MessageComposition::setIsDraft(bool isDraft)
{
	if (m_isDraft != isDraft) {
		m_isDraft = isDraft;
		Q_EMIT isDraftChanged();

		if (!isDraft) {
			MessageDb::instance()->removeDraftMessage(m_accountJid, m_chatJid);
		}
	}
}

void MessageComposition::send()
{
	Q_ASSERT(!m_accountJid.isNull());
	Q_ASSERT(!m_chatJid.isNull());

	Message message;

	message.accountJid = m_accountJid;
	message.chatJid = m_chatJid;
	message.isOwn = true;

	if (const auto rosterItem = RosterModel::instance()->findItem(m_chatJid)) {
		message.groupChatSenderId = rosterItem->groupChatParticipantId;
	}

	const auto originId = QXmppUtils::generateStanzaUuid();
	message.id = originId;
	message.originId = originId;

	message.timestamp = QDateTime::currentDateTimeUtc();
	message.body = m_body;
	message.encryption = ChatController::instance()->activeEncryption();
	message.deliveryState = DeliveryState::Pending;
	message.isSpoiler = m_spoiler;
	message.spoilerHint = m_spoilerHint;
	message.files = m_fileSelectionModel->files();
	message.receiptRequested = true;

	// generate file IDs if needed
	if (m_fileSelectionModel->hasFiles()) {
		message.fileGroupId = FileSharingController::generateFileId();

		for (auto &file : message.files) {
			file.fileGroupId = *message.fileGroupId;
			file.id = FileSharingController::generateFileId();
			file.name = QUrl::fromLocalFile(file.localFilePath).fileName();
		}
	}

	// add pending message to database
	MessageDb::instance()->addMessage(message, MessageOrigin::UserInput);

	if (m_fileSelectionModel->hasFiles()) {
		// upload files and send message after uploading

		// whether to symmetrically encrypt the files
		bool encrypt = message.encryption != Encryption::NoEncryption;

		auto *fSController = Kaidan::instance()->fileSharingController();
		// upload files
		fSController->sendFiles(message.files, encrypt).then(fSController, [message = std::move(message)](auto result) mutable {
			if (auto files = std::get_if<QVector<File>>(&result)) {
				// uploading succeeded

				// set updated files with new metadata and uploaded sources
				message.files = std::move(*files);

				// update message in database
				MessageDb::instance()->updateMessage(message.id, [files = message.files](auto &message) {
					message.files = files;
				});

				// send message with file sources
				MessageController::instance()->sendPendingMessage(std::move(message));
			} else {
				// uploading did not succeed
				auto errorText = std::get<QXmppError>(std::move(result)).description;

				// set error text in database
				MessageDb::instance()->updateMessage(message.id, [errorText](auto &message) {
					message.errorText = tr("Upload failed: %1").arg(errorText);
				});
			}
		});
		m_fileSelectionModel->clear();
	} else {
		// directly send message
		MessageController::instance()->sendPendingMessage(std::move(message));
	}

	// clean up
	setSpoiler(false);
	setIsDraft(false);
}

void MessageComposition::correct()
{
	MessageDb::instance()->updateMessage(m_replaceId, [this, replaceId = m_replaceId, body = m_body, spoilerHint = m_spoilerHint](Message &message) {
		message.body = body;
		message.isSpoiler = !spoilerHint.isEmpty();
		message.spoilerHint = spoilerHint;

		if (message.deliveryState != Enums::DeliveryState::Pending) {
			message.id = QXmppUtils::generateStanzaUuid();

			// Set "replaceId" only on first correction.
			// That way, "replaceId" stays the ID of the originally corrected message.
			if (message.replaceId.isEmpty()) {
				message.replaceId = replaceId;
			}

			message.deliveryState = Enums::DeliveryState::Pending;

			runOnThread(this, [this, message]() mutable {
				if (ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
					// the trick with the time is important for the servers
					// this way they can tell which version of the message is the latest
					message.timestamp = QDateTime::currentDateTimeUtc();

					await(MessageController::instance()->send(message.toQXmpp()), this, [messageId = message.id](QXmpp::SendResult &&result) {
						if (std::holds_alternative<QXmppError>(result)) {
							// TODO store in the database only error codes, assign text messages right in the QML
							Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Message correction was not successful"));

							MessageDb::instance()->updateMessage(messageId, [](Message &message) {
								message.deliveryState = DeliveryState::Error;
								message.errorText = QStringLiteral("Message correction was not successful");
							});
						} else {
							MessageDb::instance()->updateMessage(messageId, [](Message &message) {
								message.deliveryState = DeliveryState::Sent;
								message.errorText.clear();
							});
						}
					});
				}
			});
		} else if (message.replaceId.isEmpty()) {
			message.timestamp = QDateTime::currentDateTimeUtc();
		}
	});
}

void MessageComposition::loadDraft()
{
	if (m_accountJid.isEmpty() || m_chatJid.isEmpty()) {
		return;
	}

	auto future = MessageDb::instance()->fetchDraftMessage(m_accountJid, m_chatJid);
	await(future, this, [this](std::optional<Message> message) {
		if (message) {
			setReplaceId(message->replaceId);
			setBody(message->body);
			setSpoiler(message->isSpoiler);
			setSpoilerHint(message->spoilerHint);
			setIsDraft(true);
		}
	});
}

void MessageComposition::saveDraft()
{
	if (m_accountJid.isEmpty() || m_chatJid.isEmpty()) {
		return;
	}

	const bool savingNeeded = !m_body.isEmpty() || !m_spoilerHint.isEmpty();

	if (m_isDraft) {
		if (savingNeeded) {
			MessageDb::instance()->updateDraftMessage(m_accountJid, m_chatJid, [this](Message &message) {
				message.replaceId = m_replaceId;
				message.timestamp = QDateTime::currentDateTimeUtc();
				message.body = m_body;
				message.isSpoiler = m_spoiler;
				message.spoilerHint = m_spoilerHint;
			});
		} else {
			MessageDb::instance()->removeDraftMessage(m_accountJid, m_chatJid);
		}
	} else if (savingNeeded) {
		Message message;
		message.accountJid = m_accountJid;
		message.chatJid = m_chatJid;
		message.replaceId = m_replaceId;
		message.timestamp = QDateTime::currentDateTimeUtc();
		message.body = m_body;
		message.isSpoiler = m_spoiler;
		message.spoilerHint = m_spoilerHint;
		message.deliveryState = DeliveryState::Draft;

		MessageDb::instance()->addDraftMessage(message);
	}
}

FileSelectionModel::FileSelectionModel(QObject *parent)
	: QAbstractListModel(parent)
{
}

FileSelectionModel::~FileSelectionModel() = default;

QHash<int, QByteArray> FileSelectionModel::roleNames() const
{
	static const QHash<int, QByteArray> roles {
		{ Filename, QByteArrayLiteral("fileName") },
		{ Thumbnail, QByteArrayLiteral("thumbnail") },
		{ Description, QByteArrayLiteral("description") },
		{ FileSize, QByteArrayLiteral("fileSize") }
	};
	return roles;
}

int FileSelectionModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	}
	return m_files.size();
}

QVariant FileSelectionModel::data(const QModelIndex &index, int role) const
{
	if (!hasIndex(index.row(), index.column(), index.parent())) {
		return {};
	}

	const auto &file = m_files.at(index.row());
	switch (role) {
	case Filename:
		return QUrl::fromLocalFile(file.localFilePath).fileName();
	case Description:
		if (file.description) {
			return *file.description;
		}
		return QString();
	case Thumbnail:
		if (!file.thumbnail.isNull()) {
			return QImage::fromData(file.thumbnail);
		}
		return file.mimeType.iconName();
	case FileSize:
		if (file.size) {
			return QmlUtils::formattedDataSize(*file.size);
		}
		return tr("Unknown size");
	}
	return {};
}

void FileSelectionModel::selectFile()
{
	auto *dialog = new QFileDialog();
	dialog->setFileMode(QFileDialog::ExistingFiles);

	connect(dialog, &QFileDialog::filesSelected, this, [this, dialog]() {
		const auto files = dialog->selectedFiles();
		bool filesAdded = false;

		for (const auto &file : files) {
			if (addFile(QUrl::fromLocalFile(file))) {
				filesAdded = true;
			}
		}

		if (filesAdded) {
			Q_EMIT selectFileFinished();
		}
	});
	connect(dialog, &QDialog::finished, this, [dialog](auto) {
		dialog->deleteLater();
	});

	dialog->open();
}

bool FileSelectionModel::addFile(const QUrl &localFilePath)
{
	auto localPath = localFilePath.toLocalFile();

	bool alreadyAdded = containsIf(m_files, [=](const auto &file) {
		return file.localFilePath == localPath;
	});

	if (alreadyAdded) {
		return false;
	}

	Q_ASSERT(localFilePath.isLocalFile());
	const auto limit = Kaidan::instance()->serverFeaturesCache()->httpUploadLimit();
	const QFileInfo fileInfo(localPath);

	if (fileInfo.size() > limit) {
		Kaidan::instance()->passiveNotificationRequested(tr("'%1' cannot be sent because it is larger than %2").arg(fileInfo.fileName(), Kaidan::instance()->serverFeaturesCache()->httpUploadLimitString()));
		return false;
	}

	File file;
	file.localFilePath = localPath;
	file.mimeType = MediaUtils::mimeDatabase().mimeTypeForFile(localPath);
	file.size = fileInfo.size();

	generateThumbnail(file);

	beginInsertRows({}, m_files.size(), m_files.size());
	m_files.push_back(std::move(file));
	endInsertRows();

	return true;
}

void FileSelectionModel::removeFile(int index)
{
	beginRemoveRows({}, index, index);
	m_files.erase(m_files.begin() + index);
	endRemoveRows();
}

void FileSelectionModel::clear()
{
	beginResetModel();
	m_files.clear();
	endResetModel();
}

bool FileSelectionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	auto &file = m_files[index.row()];

	switch (role) {
	case Description:
		file.description = value.toString();
		break;
	default:
		return false;
	}

	return true;
}

void FileSelectionModel::generateThumbnail(const File &file)
{
	static auto allPlugins = KIO::PreviewJob::availablePlugins();
	KFileItemList items {
		KFileItem {
			QUrl::fromLocalFile(file.localFilePath),
			QMimeDatabase().mimeTypeForFile(file.localFilePath).name(),
		}
	};
	auto *job = new KIO::PreviewJob(items, QSize(THUMBNAIL_SIZE, THUMBNAIL_SIZE), &allPlugins);
	job->setAutoDelete(true);

	connect(job, &KIO::PreviewJob::gotPreview, this, [this](const KFileItem &item, const QPixmap &preview) {
		auto *file = std::find_if(m_files.begin(), m_files.end(), [&](const auto &file) {
			return file.localFilePath == item.localPath();
		});

		if (file != m_files.cend()) {
			file->thumbnail = MediaUtils::encodeImageThumbnail(preview);
			int i = std::distance(m_files.begin(), file);
			Q_EMIT dataChanged(index(i), index(i), { Thumbnail });
		}
	});
	connect(job, &KIO::PreviewJob::failed, this, [](const KFileItem &item) {
		qDebug() << "Could not generate a thumbnail for" << item.url();
	});
	job->start();
}

const QVector<File> &FileSelectionModel::files() const
{
	return m_files;
}

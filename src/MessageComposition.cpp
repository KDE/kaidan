// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageComposition.h"

// Kaidan
#include "MessageHandler.h"
#include "Kaidan.h"
#include "FileSharingController.h"
#include "Algorithms.h"
#include "MessageModel.h"
#include "MediaUtils.h"
#include "MessageDb.h"

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

	Message message {
		.accountJid = m_accountJid,
		.chatJid = m_chatJid,
		.senderId = m_accountJid,
		.id = QXmppUtils::generateStanzaUuid(),
		.originId = message.id,
		.stanzaId = {},
		.replaceId = {},
		.timestamp = QDateTime::currentDateTimeUtc(),
		.body = m_body,
		.encryption = MessageModel::instance()->activeEncryption(),
		.senderKey = {},
		.deliveryState = DeliveryState::Pending,
		.isSpoiler = m_spoiler,
		.spoilerHint = m_spoilerHint,
		.fileGroupId = {},
		.files = m_fileSelectionModel->files(),
		.markerId = {},
		.receiptRequested = true,
		.reactionSenders = {},
		.errorText = {},
	};

	if (m_fileSelectionModel->hasFiles()) {
		bool encrypt = message.encryption != Encryption::NoEncryption;
		Kaidan::instance()->fileSharingController()->sendMessage(std::move(message), encrypt);
		m_fileSelectionModel->clear();
	} else {
		MessageDb::instance()->addMessage(message, MessageOrigin::UserInput);

		auto *client = Kaidan::instance()->client();
		runOnThread(client, [client, message = std::move(message)]() mutable {
			client->messageHandler()->sendPendingMessage(std::move(message));
		});
	}

	setSpoiler(false);
	setIsDraft(false);
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
			return QLocale::system().formattedDataSize(*file.size);
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
		Q_EMIT selectFileFinished();

		const auto files = dialog->selectedFiles();
		for (const auto &file : files) {
			addFile(QUrl::fromLocalFile(file));
		}
	});
	connect(dialog, &QDialog::finished, this, [dialog](auto) {
		dialog->deleteLater();
	});

	dialog->open();
}

void FileSelectionModel::addFile(const QUrl &localFilePath)
{
	auto localPath = localFilePath.toLocalFile();

	bool alreadyAdded = containsIf(m_files, [=](const auto &file) {
		return file.localFilePath == localPath;
	});

	if (alreadyAdded) {
		return;
	}

	Q_ASSERT(localFilePath.isLocalFile());
	File file;
	file.localFilePath = localPath;
	file.mimeType = MediaUtils::mimeDatabase().mimeTypeForFile(localPath);
	file.size = QFileInfo(localPath).size();

	generateThumbnail(file);

	beginInsertRows({}, m_files.size(), m_files.size());
	m_files.push_back(std::move(file));
	endInsertRows();
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

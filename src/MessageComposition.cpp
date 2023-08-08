// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageComposition.h"

// std
// Kaidan
#include "MessageHandler.h"
#include "Kaidan.h"
#include "FileSharingController.h"
#include "Algorithms.h"
#include "AccountManager.h"
#include "MessageModel.h"
#include "MediaUtils.h"
#include "MessageDb.h"

#include <QFileDialog>
#include <QFutureWatcher>
#include <QMimeDatabase>

#include <KIO/PreviewJob>
#include <KFileItem>

constexpr auto THUMBNAIL_SIZE = 200;

MessageComposition::MessageComposition()
	: m_fileSelectionModel(new FileSelectionModel(this))
	, m_fetchDraftWatcher(new QFutureWatcher<Message>(this))
	, m_removeDraftWatcher(new QFutureWatcher<QString>(this))
{
	connect(m_fetchDraftWatcher, &QFutureWatcher<Message>::finished, this, [this]() {
		const auto msg = m_fetchDraftWatcher->result();
		emit draftFetched(msg.body, msg.isSpoiler, msg.spoilerHint);
	});
	connect(m_removeDraftWatcher, &QFutureWatcher<QString>::finished, this, [this]() {
		setDraftId({});
	});
}

void MessageComposition::setAccount(const QString &account)
{
	if (m_account != account) {
		m_account = account;
		emit accountChanged();
	}
}

void MessageComposition::setTo(const QString &to)
{
	if (m_to != to) {
		m_to = to;
		emit toChanged();
	}
}

void MessageComposition::setBody(const QString &body)
{
	if (m_body != body) {
		m_body = body;
		emit bodyChanged();
	}
}

void MessageComposition::setSpoiler(bool spoiler)
{
	if (m_spoiler != spoiler) {
		m_spoiler = spoiler;
		emit isSpoilerChanged();
	}
}

void MessageComposition::setSpoilerHint(const QString &spoilerHint)
{
	if (m_spoilerHint != spoilerHint) {
		m_spoilerHint = spoilerHint;
		emit spoilerHintChanged();
	}
}

void MessageComposition::setDraftId(const QString &id)
{
	if (m_draftId != id) {
		m_draftId = id;
		emit draftIdChanged();

		if (!m_draftId.isEmpty()) {
			m_fetchDraftWatcher->setFuture(MessageDb::instance()->fetchDraftMessage(id));
		}
	}
}

void MessageComposition::send()
{
	Q_ASSERT(!m_account.isNull());
	Q_ASSERT(!m_to.isNull());

	if (m_fileSelectionModel->hasFiles()) {
		Message message;
		message.to = m_to;
		message.from = AccountManager::instance()->jid();
		message.body = m_body;
		message.files = m_fileSelectionModel->files();
		message.receiptRequested = true;
		message.encryption = MessageModel::instance()->activeEncryption();

		bool encrypt = message.encryption != Encryption::NoEncryption;
		Kaidan::instance()->fileSharingController()->sendMessage(std::move(message), encrypt);
		m_fileSelectionModel->clear();
	} else {
		emit Kaidan::instance()
			->client()
			->messageHandler()
			->sendMessageRequested(m_to, m_body, m_spoiler, m_spoilerHint);
	}

	setSpoiler(false);

	if (!m_draftId.isEmpty()) {
		m_removeDraftWatcher->setFuture(MessageDb::instance()->removeDraftMessage(m_draftId));
	}
}

Message MessageComposition::draft() const
{
	Message msg;
	msg.id = m_draftId;
	msg.deliveryState = DeliveryState::Draft;
	msg.from = m_account;
	msg.to = m_to;
	msg.isSpoiler = m_spoiler;
	msg.spoilerHint = m_spoilerHint;
	msg.body = m_body;
	return msg;
}

void MessageComposition::saveDraft()
{
	if (m_account.isEmpty() || m_to.isEmpty()) {
		return;
	}

	const Message msg = draft();
	const bool canSave = !msg.spoilerHint.isEmpty() || !msg.body.isEmpty();

	if (msg.id.isEmpty()) {
		if (canSave) {
			MessageDb::instance()->addDraftMessage(msg);
		}
	} else {
		if (canSave) {
			MessageDb::instance()->updateDraftMessage(msg);
		} else {
			m_removeDraftWatcher->setFuture(MessageDb::instance()->removeDraftMessage(msg.id));
		}
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

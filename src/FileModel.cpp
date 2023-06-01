// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FileModel.h"

#include "MessageDb.h"

FileModel::FileModel(QObject *parent) : QAbstractListModel(parent)
{
	connect(&m_watcher, &QFutureWatcher<Files>::finished, this, [this]() {
		setFiles(m_watcher.result());
	});
}

FileModel::~FileModel()
{
	m_watcher.cancel();
	m_watcher.waitForFinished();
}

int FileModel::rowCount(const QModelIndex &parent) const
{
	return parent == QModelIndex() ? m_files.count() : 0;
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
	if (hasIndex(index.row(), index.column())) {
		const auto &file = m_files[index.row()];

		switch (role) {
		case Qt::DisplayRole:
			return file.httpSources.isEmpty()
					? QString()
					: file.httpSources.constBegin()->url.toDisplayString();
		case static_cast<int>(Role::Id):
			return file.id;
		case static_cast<int>(Role::File):
			return QVariant::fromValue(file);
		}
	}

	return {};
}

QHash<int, QByteArray> FileModel::roleNames() const
{
	static const auto roles = [this]() {
		// The ID is used by C++ but QML does not understand qint64.
		// Thus, it is added to "data()" but not here.
		auto roles = QAbstractListModel::roleNames();
		roles.insert(static_cast<int>(Role::File), QByteArrayLiteral("file"));
		return roles;
	}();
	return roles;
}

QString FileModel::accountJid() const
{
	return m_accountJid;
}

void FileModel::setAccountJid(const QString &jid)
{
	if (m_accountJid != jid) {
		m_accountJid = jid;
		Q_EMIT accountJidChanged(m_accountJid);
	}
}

QString FileModel::chatJid() const
{
	return m_chatJid;
}

void FileModel::setChatJid(const QString &jid)
{
	if (m_chatJid != jid) {
		m_chatJid = jid;
		Q_EMIT chatJidChanged(m_chatJid);
	}
}

Files FileModel::files() const
{
	return m_files;
}

void FileModel::setFiles(const Files &files)
{
	if (m_files != files) {
		beginResetModel();
		m_files = files;
		endResetModel();

		Q_EMIT rowCountChanged();
	}
}

#ifndef BUILD_TESTS
void FileModel::loadFiles()
{
	m_watcher.cancel();

	if (m_accountJid.isEmpty()) {
		qWarning("FileModel: Trying to call loadFiles() but m_accountJid is empty.");
	} else {
		m_watcher.setFuture(MessageDb::instance()->fetchFiles(m_accountJid, m_chatJid));
	}
}

void FileModel::loadDownloadedFiles()
{
	m_watcher.cancel();

	if (m_accountJid.isEmpty()) {
		qWarning(
			"FileModel: Trying to call loadDownloadedFiles() but m_accountJid is "
			"empty.");
	} else {
		m_watcher.setFuture(MessageDb::instance()->fetchDownloadedFiles(m_accountJid, m_chatJid));
	}
}
#endif

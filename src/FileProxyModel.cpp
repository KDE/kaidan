// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FileProxyModel.h"

#include "FileModel.h"

#include <QFile>
#include <QtConcurrentRun>

FileProxyModel::FileProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent)
{
}

Qt::ItemFlags FileProxyModel::flags(const QModelIndex &index) const
{
	auto flags = QSortFilterProxyModel::flags(index);

	if (index.isValid()) {
		flags |= Qt::ItemIsUserCheckable;
	}

	return flags;
}

QVariant FileProxyModel::data(const QModelIndex &index, int role) const
{
	if (hasIndex(index.row(), index.column(), index.parent())) {
		if (role == Qt::CheckStateRole) {
			const auto id = index.data(static_cast<int>(FileModel::Role::Id)).value<qint64>();
			return m_checkedIds.contains(id) ? Qt::Checked : Qt::Unchecked;
		}
	}

	return QSortFilterProxyModel::data(index, role);
}

bool FileProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (data(index, role) != value) {
		if (role == Qt::CheckStateRole) {
			const auto id = index.data(static_cast<int>(FileModel::Role::Id)).value<qint64>();

			if (value.toBool()) {
				m_checkedIds.insert(id);
			} else {
				m_checkedIds.remove(id);
			}

			Q_EMIT dataChanged(index, index, { role });
			Q_EMIT checkedCountChanged();
			return true;
		}
	}

	return false;
}

QHash<int, QByteArray> FileProxyModel::roleNames() const
{
	static const auto roles = [this]() {
		auto roles = QSortFilterProxyModel::roleNames();
		roles.insert(Qt::CheckStateRole, QByteArrayLiteral("checkState"));
		return roles;
	}();
	return roles;
}

FileProxyModel::Mode FileProxyModel::mode() const
{
	return m_mode;
}

void FileProxyModel::setMode(Mode mode)
{
	if (m_mode != mode) {
		m_mode = mode;
		m_checkedIds.clear();
		invalidateFilter();
		Q_EMIT modeChanged();
		Q_EMIT rowCountChanged();
		Q_EMIT checkedCountChanged();
	}
}

int FileProxyModel::checkedCount() const
{
	return m_checkedIds.count();
}

void FileProxyModel::checkAll()
{
	Q_EMIT layoutAboutToBeChanged();

	const auto count = rowCount();

	m_checkedIds.reserve(count);

	for (int i = 0; i < count; ++i) {
		const QModelIndex index = QSortFilterProxyModel::index(i, 0);
		m_checkedIds.insert(index.data(static_cast<int>(FileModel::Role::Id)).toLongLong());
	}

	Q_EMIT layoutChanged();
	Q_EMIT checkedCountChanged();
}

void FileProxyModel::clearChecked()
{
	if (!m_checkedIds.isEmpty()) {
		Q_EMIT layoutAboutToBeChanged();

		m_checkedIds.clear();

		Q_EMIT layoutChanged();
		Q_EMIT checkedCountChanged();
	}
}

void FileProxyModel::deleteChecked()
{
	if (!m_checkedIds.isEmpty()) {
		QStringList files;

		files.reserve(rowCount());

		for (int i = 0; i < rowCount(); ++i) {
			const QModelIndex index = FileProxyModel::index(i, 0);
			const File file = index.data(static_cast<int>(FileModel::Role::File)).value<File>();

			if (m_checkedIds.contains(file.id)) {
				files.append(file.localFilePath);
			}
		}

		QtConcurrent::run([this, files = std::move(files)]() mutable {
			QStringList errors;

			errors.reserve(files.count());

			for (int i = files.count() - 1; i >= 0; --i) {
				QFile file(files[i]);

				if (!file.remove()) {
					files.removeAt(i);
					errors.append(QStringLiteral("%1: %2").arg(file.fileName(), file.errorString()));
				}
			}

			QMetaObject::invokeMethod(this, [this, files = std::move(files), errors = std::move(errors)]() {
				_filesDeleted(files, errors);
			});
		});
	}
}

bool FileProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	const QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
	const auto file = sourceIndex.data(static_cast<int>(FileModel::Role::File)).value<File>();

	if (QFile::exists(file.localFilePath)) {
		switch (m_mode) {
		case Mode::All:
			return true;
		case Mode::Images:
			return filterAcceptsImage(file);
		case Mode::Videos:
			return filterAcceptsVideo(file);
		case Mode::Other:
			return filterAcceptsOther(file);
		}

		Q_UNREACHABLE();
	}

	return false;
}

bool FileProxyModel::filterAcceptsImage(const File &file) const
{
	if (file.mimeType.isValid()) {
		return file.mimeType.name().startsWith(QStringLiteral("image/"));
	}

	return false;
}

bool FileProxyModel::filterAcceptsVideo(const File &file) const
{
	if (file.mimeType.isValid()) {
		return file.mimeType.name().startsWith(QStringLiteral("video/"));
	}

	return false;
}

bool FileProxyModel::filterAcceptsOther(const File &file) const
{
	return !filterAcceptsImage(file) && !filterAcceptsVideo(file);
}

void FileProxyModel::_filesDeleted(const QStringList &files, const QStringList &errors)
{
	m_checkedIds.clear();
	invalidateFilter();

	Q_EMIT rowCountChanged();
	Q_EMIT checkedCountChanged();
	Q_EMIT filesDeleted(files, errors);
}

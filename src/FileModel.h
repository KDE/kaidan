// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Message.h"

#include <QAbstractListModel>
#include <QFutureWatcher>

using Files = QVector<File>;

class FileModel : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(QString accountJid READ accountJid WRITE setAccountJid NOTIFY accountJidChanged)
	Q_PROPERTY(QString chatJid READ chatJid WRITE setChatJid NOTIFY chatJidChanged)
	Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)

public:
	enum class Role {
		Id = Qt::UserRole,
		File
	};
	Q_ENUM(Role)

	explicit FileModel(QObject *parent = nullptr);
	~FileModel() override;

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	QString accountJid() const;
	void setAccountJid(const QString &jid);
	Q_SIGNAL void accountJidChanged(const QString &jid);

	QString chatJid() const;
	void setChatJid(const QString &jid);
	Q_SIGNAL void chatJidChanged(const QString &jid);

	Files files() const;
	void setFiles(const Files &files);

#ifndef BUILD_TESTS
	Q_SLOT void loadFiles();
	Q_SLOT void loadDownloadedFiles();
#endif

	Q_SIGNAL void rowCountChanged();

private:
	QString m_accountJid;
	QString m_chatJid;
	Files m_files;
	QFutureWatcher<Files> m_watcher;
};

Q_DECLARE_METATYPE(FileModel::Role)

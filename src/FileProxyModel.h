// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>
#include <QSet>

struct File;

class FileProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	Q_PROPERTY(FileProxyModel::Mode mode READ mode WRITE setMode NOTIFY modeChanged)
	Q_PROPERTY(int checkedCount READ checkedCount NOTIFY checkedCountChanged)
	Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)

public:
	enum class Mode {
		All,
		Images,
		Videos,
		Other
	};
	Q_ENUM(Mode)

	explicit FileProxyModel(QObject *parent = nullptr);

	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value,
				 int role = Qt::EditRole) override;
	QHash<int, QByteArray> roleNames() const override;

	Mode mode() const;
	void setMode(Mode mode);
	Q_SIGNAL void modeChanged();

	int checkedCount() const;
	Q_SIGNAL void checkedCountChanged();

	Q_SLOT void checkAll();
	Q_SLOT void clearChecked();

	Q_SLOT void deleteChecked();
	Q_SIGNAL void filesDeleted(const QStringList &files, const QStringList &errors);

	Q_SIGNAL void rowCountChanged();

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
	bool filterAcceptsImage(const File &file) const;
	bool filterAcceptsVideo(const File &file) const;
	bool filterAcceptsOther(const File &file) const;
	void _filesDeleted(const QStringList &files, const QStringList &errors);

private:
	Mode m_mode = Mode::All;
	QSet<qint64> m_checkedIds;
};

Q_DECLARE_METATYPE(FileProxyModel::Mode)


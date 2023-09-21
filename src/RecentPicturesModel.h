// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtGlobal>

#ifdef Q_OS_ANDROID
#include <QAbstractListModel>

class RecentPicturesModel : public QAbstractListModel
{
	Q_OBJECT

public:
	explicit RecentPicturesModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &) const override;
	QVariant data(const QModelIndex &, int) const override;

};
#else
#include <KDirSortFilterProxyModel>

class RecentPicturesModel : public KDirSortFilterProxyModel
{
	Q_OBJECT

	enum Role {
		FilePath = Qt::UserRole + 1
	};

public:
	explicit RecentPicturesModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role) const override;

    bool subSortLessThan(const QModelIndex &left, const QModelIndex &right) const override;
};
#endif

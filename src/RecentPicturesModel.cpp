// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RecentPicturesModel.h"

#ifdef Q_OS_ANDROID
RecentPicturesModel::RecentPicturesModel(QObject *parent)
	: QAbstractListModel(parent)
{
}

int RecentPicturesModel::rowCount(const QModelIndex &) const
{
	return 0;
}

QVariant RecentPicturesModel::data(const QModelIndex &, int) const
{
	return {};
}
#else
#include <KDirLister>
#include <KDirModel>

RecentPicturesModel::RecentPicturesModel(QObject *parent)
    : KDirSortFilterProxyModel{parent}
{
	auto *dirModel = new KDirModel(this);
	setSourceModel(dirModel);
	dirModel->dirLister()->setMimeFilter({QStringLiteral("image/png"), QStringLiteral("image/jpeg")});
	dirModel->openUrl(QUrl(QStringLiteral("recentlyused:/files/")));
	dirModel->dirLister()->setAutoErrorHandlingEnabled(false);
}

QHash<int, QByteArray> RecentPicturesModel::roleNames() const {
	return {
		{Role::FilePath, "filePath"}
	};
}

QVariant RecentPicturesModel::data(const QModelIndex &index, int role) const {
	if (role == Role::FilePath) {
		auto fileItem = KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole).value<KFileItem>();
		return fileItem.mostLocalUrl();
	}

	return KDirSortFilterProxyModel::data(index, role);
}

bool RecentPicturesModel::subSortLessThan(const QModelIndex &left, const QModelIndex &right) const
{
	auto leftFile = left.data(KDirModel::FileItemRole).value<KFileItem>();
	auto rightFile = right.data(KDirModel::FileItemRole).value<KFileItem>();

	return leftFile.time(KFileItem::ModificationTime) > rightFile.time(KFileItem::ModificationTime);
}
#endif

// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCameraInfo>
#include <QAbstractListModel>

class CameraInfo : public QCameraInfo {
	Q_GADGET

	Q_PROPERTY(bool isNull READ isNull CONSTANT)
	Q_PROPERTY(QString deviceName READ deviceName CONSTANT)
	Q_PROPERTY(QString description READ description CONSTANT)
	Q_PROPERTY(QCamera::Position position READ position CONSTANT)
	Q_PROPERTY(int orientation READ orientation CONSTANT)

public:
	using QCameraInfo::QCameraInfo;
	explicit CameraInfo(const QString &deviceName);
	CameraInfo() = default;
	CameraInfo(const QCameraInfo &other);
};

class CameraModel : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(int rowCount READ rowCount NOTIFY camerasChanged)
	Q_PROPERTY(QList<QCameraInfo> cameras READ cameras NOTIFY camerasChanged)
	Q_PROPERTY(CameraInfo defaultCamera READ defaultCamera NOTIFY camerasChanged)
	Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
	Q_PROPERTY(CameraInfo currentCamera READ currentCamera WRITE setCurrentCamera NOTIFY currentIndexChanged)

public:
	enum CustomRoles {
		IsNullRole = Qt::UserRole,
		DeviceNameRole,
		DescriptionRole,
		PositionRole,
		OrientationRole,
		CameraInfoRole
	};

	explicit CameraModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	QList<QCameraInfo> cameras() const;
	static CameraInfo defaultCamera();

	int currentIndex() const;
	void setCurrentIndex(int currentIndex);

	CameraInfo currentCamera() const;
	void setCurrentCamera(const CameraInfo &currentCamera);

	Q_INVOKABLE CameraInfo camera(int row) const;
	Q_INVOKABLE int indexOf(const QString &deviceName) const;

	static CameraInfo camera(const QString &deviceName);
	static CameraInfo camera(QCamera::Position position);

	Q_INVOKABLE void refresh();

	Q_SIGNAL void camerasChanged();
	Q_SIGNAL void currentIndexChanged();

private:
	QList<QCameraInfo> m_cameras;
	int m_currentIndex = -1;
};

Q_DECLARE_METATYPE(CameraInfo)

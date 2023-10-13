// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QList>

#include <functional>

template <typename T>
class MediaSettingModel : public QAbstractListModel
{
public:
	enum CustomRoles {
		ValueRole = Qt::UserRole,
		DescriptionRole
	};

	using ToString = std::function<QString(const T &, const void *userData)>;

	using QAbstractListModel::QAbstractListModel;

	explicit MediaSettingModel(MediaSettingModel::ToString toString,
		const void *userData = nullptr, QObject *parent = nullptr)
		: QAbstractListModel(parent)
		  , m_toString(toString)
		  , m_userData(userData) {
	}

	int rowCount(const QModelIndex &parent = QModelIndex()) const override {
		return parent == QModelIndex() ? m_values.count() : 0;
	}

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
		if (hasIndex(index.row(), index.column(), index.parent())) {
			switch (role) {
			case MediaSettingModel::CustomRoles::ValueRole:
				return QVariant::fromValue(m_values[index.row()]);
			case MediaSettingModel::CustomRoles::DescriptionRole:
				return toString(m_values[index.row()]);
			}
		}

		return { };
	}

	QHash<int, QByteArray> roleNames() const override {
		static const QHash<int, QByteArray> roles {
			{ MediaSettingModel::CustomRoles::ValueRole, QByteArrayLiteral("value") },
			{ MediaSettingModel::CustomRoles::DescriptionRole, QByteArrayLiteral("description") }
		};

		return roles;
	}

	MediaSettingModel::ToString toString() const {
		return m_toString;
	}

	void setToString(MediaSettingModel::ToString toString) {
		m_toString = toString;

		Q_EMIT toStringChanged();

		const int count = rowCount();

		if (count > 0) {
			Q_EMIT dataChanged(index(0, 0), index(count -1, 0));
		}
	}

	const void *userData() const {
		return m_userData;
	}

	void setUserData(const void *userData) {
		if (m_userData == userData) {
			return;
		}

		m_userData = userData;

		Q_EMIT userDataChanged();

		const int count = rowCount();

		if (count > 0) {
			Q_EMIT dataChanged(index(0, 0), index(count -1, 0));
		}
	}

	QList<T> values() const {
		return m_values;
	}

	void setValues(const QList<T> &values) {
		if (m_values == values) {
			return;
		}

		const int newCurrentIndex = m_currentIndex != -1 ? values.indexOf(currentValue()) : -1;
		const bool curIdxChanged = newCurrentIndex != m_currentIndex;

		beginResetModel();
		m_values = values;
		m_currentIndex = newCurrentIndex;
		endResetModel();

		Q_EMIT valuesChanged();

		if (curIdxChanged) {
			Q_EMIT currentIndexChanged();
		}
	}

	int currentIndex() const {
		return m_currentIndex;
	}

	void setCurrentIndex(int currentIndex) {
		if (currentIndex < 0 || currentIndex >= m_values.count()
			|| m_currentIndex == currentIndex) {
			return;
		}

		m_currentIndex = currentIndex;
		Q_EMIT currentIndexChanged();
	}

	T currentValue() const {
		return m_currentIndex >= 0 && m_currentIndex < m_values.count()
			       ? m_values[m_currentIndex]
			       : T();
	}

	void setCurrentValue(const T &currentValue) {
		setCurrentIndex(indexOf(currentValue));
	}

	QString currentDescription() const {
		return m_currentIndex >= 0 && m_currentIndex < m_values.count()
			       ? toString(currentValue())
			       : QString();
	}

	void setValuesAndCurrentIndex(const QList<T> &values, int currentIndex) {
		if (m_values == values && m_currentIndex == currentIndex) {
			return;
		}

		beginResetModel();
		m_values = values;
		m_currentIndex = currentIndex >= 0 && currentIndex < m_values.count()
					 ? currentIndex
					 : -1;
		endResetModel();

		Q_EMIT valuesChanged();
		Q_EMIT currentIndexChanged();
	}

	void setValuesAndCurrentValue(const QList<T> &values, const T &currentValue) {
		setValuesAndCurrentIndex(values, values.indexOf(currentValue));
	}

	// Invokables
	virtual void clear() {
		beginResetModel();
		m_currentIndex = -1;
		m_values.clear();
		endResetModel();

		Q_EMIT valuesChanged();
		Q_EMIT currentIndexChanged();
	}

	virtual int indexOf(const T &value) const {
		return m_values.indexOf(value);
	}

	virtual T value(int index) const {
		if (index < 0 || index >= m_values.count()) {
			return { };
		}

		return m_values[index];
	}

	virtual QString description(int index) const {
		if (index < 0 || index >= m_values.count()) {
			return { };
		}

		return toString(m_values[index]);
	}

	virtual QString toString(const T &value) const {
		if (m_toString) {
			return m_toString(value, m_userData);
		}

		return QVariant::fromValue(value).toString();
	}

	// Signals
	virtual void toStringChanged() = 0;
	virtual void userDataChanged() = 0;
	virtual void valuesChanged() = 0;
	virtual void currentIndexChanged() = 0;

private:
	MediaSettingModel::ToString m_toString;
	const void *m_userData = nullptr;
	int m_currentIndex = -1;
	QList<T> m_values;
};

#define DECL_MEDIA_SETTING_MODEL(NAME, TYPE, TO_STRING)										\
class MediaSettings##NAME##Model : public MediaSettingModel<TYPE> {								\
	Q_OBJECT														\
																\
	Q_PROPERTY(MediaSettings##NAME##Model::ToString toString READ toString WRITE setToString NOTIFY toStringChanged)	\
	Q_PROPERTY(const void *userData READ userData WRITE setUserData NOTIFY userDataChanged)					\
	Q_PROPERTY(QList<TYPE> values READ values WRITE setValues NOTIFY valuesChanged)						\
	Q_PROPERTY(int rowCount READ rowCount NOTIFY valuesChanged)								\
	Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)				\
	Q_PROPERTY(TYPE currentValue READ currentValue WRITE setCurrentValue NOTIFY currentIndexChanged)			\
	Q_PROPERTY(QString currentDescription READ currentDescription NOTIFY currentIndexChanged)				\
																\
	using MSMT = MediaSettingModel<TYPE>;											\
																\
public:																\
	using MSMT::MSMT;													\
	explicit MediaSettings##NAME##Model(const void *userData, QObject *parent = nullptr)					\
		: MSMT(TO_STRING, userData, parent)										\
	{ }															\
																\
	explicit MediaSettings##NAME##Model(QObject *parent = nullptr)								\
		: MSMT(parent)													\
	{ }															\
																\
	using MSMT::toString;													\
	Q_INVOKABLE void clear() override { MSMT::clear(); }									\
	Q_INVOKABLE int indexOf(const TYPE &value) const override { return MSMT::indexOf(value); }				\
	Q_INVOKABLE TYPE value(int index) const override { return MSMT::value(index); }						\
	Q_INVOKABLE QString description(int index) const override { return MSMT::description(index); }				\
	Q_INVOKABLE QString toString(const TYPE &value) const override { return MSMT::toString(value); }			\
																\
Q_SIGNALS:															\
	void toStringChanged() override;											\
	void userDataChanged() override;											\
	void valuesChanged() override;												\
	void currentIndexChanged() override;											\
}

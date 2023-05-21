// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DataFormModel.h"
// Qt
#include <QUrl>
// QXmpp
#include <QXmppBitsOfBinaryContentId.h>
// Kaidan
#include "Globals.h"

DataFormModel::DataFormModel(QObject *parent)
	: QAbstractListModel(parent)
{
}

DataFormModel::DataFormModel(const QXmppDataForm &dataForm, QObject *parent)
	: QAbstractListModel(parent), m_form(dataForm)
{
}

DataFormModel::~DataFormModel() = default;

int DataFormModel::rowCount(const QModelIndex &parent) const
{
	// For list models only the root node (an invalid parent) should return the list's size. For all
	// other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
	if (parent.isValid())
		return 0;

	return m_form.fields().size();
}

QVariant DataFormModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || !hasIndex(index.row(), index.column(), index.parent()))
		return {};

	const QXmppDataForm::Field &field = m_form.fields().at(index.row());

	switch(role) {
	case Key:
		return field.key();
	case Type:
		return field.type();
	case Label:
		return field.label();
	case IsRequired:
		return field.isRequired();
	case Value:
		return field.value();
	case Description:
		return field.description();
	case MediaUrl:
		return mediaSourceUri(field);
	}

	return {};
}

bool DataFormModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || !hasIndex(index.row(), index.column(), index.parent()))
		return false;

	QXmppDataForm::Field field = m_form.fields().at(index.row());

	switch(role) {
	case Key:
		field.setKey(value.toString());
		break;
	case Type:
		field.setType(static_cast<QXmppDataForm::Field::Type>(value.toInt()));
		break;
	case Label:
		field.setLabel(value.toString());
		break;
	case IsRequired:
		field.setRequired(value.toBool());
		break;
	case Value:
		field.setValue(value);
		break;
	case Description:
		field.setDescription(value.toString());
		break;
	default:
		return false;
	}

	m_form.fields()[index.row()] = field;
	emit dataChanged(index, index, QVector<int>() << role);
	return true;
}

QHash<int, QByteArray> DataFormModel::roleNames() const
{
	return {
		{Key, QByteArrayLiteral("key")},
		{Type, QByteArrayLiteral("type")},
		{Label, QByteArrayLiteral("label")},
		{IsRequired, QByteArrayLiteral("isRequired")},
		{Value, QByteArrayLiteral("value")},
		{Description, QByteArrayLiteral("description")},
		{MediaUrl, QByteArrayLiteral("mediaUrl")}
	};
}

QXmppDataForm DataFormModel::form() const
{
	return m_form;
}

void DataFormModel::setForm(const QXmppDataForm &form)
{
	beginResetModel();
	m_form = form;
	endResetModel();

	emit formChanged();
}

QString DataFormModel::instructions() const
{
	return m_form.instructions();
}

QString DataFormModel::mediaSourceUri(const QXmppDataForm::Field &field) const
{
	QString mediaSourceUri;
	const auto mediaSources = field.mediaSources();

	for (const auto &mediaSource : mediaSources) {
		mediaSourceUri = mediaSource.uri().toString();
		// Prefer Bits of Binary URIs.
		// In most cases, the data has been received already then.
		if (QXmppBitsOfBinaryContentId::isBitsOfBinaryContentId(mediaSourceUri, true))
			return QStringLiteral("image://%1/%2").arg(BITS_OF_BINARY_IMAGE_PROVIDER_NAME, mediaSourceUri);
	}

	return mediaSourceUri;
}

QString DataFormModel::title() const
{
	return m_form.title();
}

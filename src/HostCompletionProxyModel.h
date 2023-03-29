// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>

class HostCompletionProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	Q_PROPERTY(QString userInput READ userInput WRITE setUserInput NOTIFY userInputChanged)

public:
	explicit HostCompletionProxyModel(QObject *parent = nullptr);

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	QString userInput() const;
	Q_SLOT void setUserInput(const QString &userInput);
	Q_SIGNAL void userInputChanged(const QString &userInput);

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
	QString prefix() const;
	QString domain() const;

private:
	QString m_userInput;
};

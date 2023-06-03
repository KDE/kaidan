// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QSet>

class Emoji
{
	Q_GADGET

	Q_PROPERTY(const QString &unicode READ unicode CONSTANT)
	Q_PROPERTY(const QString &shortName READ shortName CONSTANT)
	Q_PROPERTY(Emoji::Group group READ group CONSTANT)

public:
	enum class Group {
		Invalid = -1,
		Favorites,
		People,
		Nature,
		Food,
		Activity,
		Travel,
		Objects,
		Symbols,
		Flags
	};
	Q_ENUM(Group)

	Emoji(const QString& u = {}, const QString& s = {}, Emoji::Group g = Emoji::Group::Invalid)
		: m_unicode(u)
		, m_shortName(s)
		, m_group(g) {
	}

	inline const QString &unicode() const { return m_unicode; }
	inline const QString &shortName() const { return m_shortName; }
	inline Emoji::Group group() const { return m_group; }

private:
	QString m_unicode;
	QString m_shortName;
	Emoji::Group m_group;
};

class EmojiModel : public QAbstractListModel
{
	Q_OBJECT

public:
	enum class Roles {
		Unicode = Qt::UserRole,
		ShortName,
		Group,
		Emoji,
	};

	using QAbstractListModel::QAbstractListModel;

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;
};

class EmojiProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	Q_PROPERTY(Emoji::Group group READ group WRITE setGroup NOTIFY groupChanged)
	Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
	Q_PROPERTY(bool hasFavoriteEmojis READ hasFavoriteEmojis NOTIFY hasFavoriteEmojisChanged)

public:
	explicit EmojiProxyModel(QObject *parent = nullptr);
	~EmojiProxyModel() override;

	Emoji::Group group() const;
	void setGroup(Emoji::Group group);

	QString filter() const;
	void setFilter(const QString &filter);

	bool hasFavoriteEmojis() const;

	Q_INVOKABLE void addFavoriteEmoji(int proxyRow);

	Q_SIGNAL void groupChanged();
	Q_SIGNAL void filterChanged();
	Q_SIGNAL void hasFavoriteEmojisChanged();

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
	Emoji::Group m_group = Emoji::Group::Invalid;
	QSet<QString> m_favoriteEmojis;
};

Q_DECLARE_METATYPE(Emoji)

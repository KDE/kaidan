// SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <optional>
// Qt
#include <QColor>
#include <QObject>
// QXmpp
#include <QXmppPresence.h>

class Presence
{
	Q_GADGET
public:
	enum Availability { Offline, Online, Away, XA, DND, Chat };
	Q_ENUM(Availability)

	static constexpr Availability availabilityFromAvailabilityStatusType(
		QXmppPresence::AvailableStatusType type)
	{
		switch (type) {
		case QXmppPresence::Online:
			return Online;
		case QXmppPresence::Away:
			return Away;
		case QXmppPresence::XA:
			return XA;
		case QXmppPresence::DND:
			return DND;
		case QXmppPresence::Chat:
			return Chat;
		default:
			return Offline;
		}
	}

	static QString availabilityToIcon(Availability type);
	static QString availabilityToText(Availability type);
	static QColor availabilityToColor(Availability type);
};

/**
 * @class PresenceCache A cache for presence holders for certain JIDs
 */
class PresenceCache : public QObject
{
	Q_OBJECT

public:
	enum ChangeType : quint8 {
		Connected,
		Disconnected,
		Updated,
	};
	Q_ENUM(ChangeType)

	PresenceCache(QObject *parent = nullptr);
	~PresenceCache();

	static PresenceCache *instance()
	{
		return s_instance;
	}

	QString pickIdealResource(const QString &jid);
	QList<QString> resources(const QString &jid);
	int resourcesCount(const QString &jid);
	std::optional<QXmppPresence> presence(const QString &jid, const QString &resource);

	/**
	 * Updates the presence cache, it will ignore subscribe presences
	 */
	void updatePresence(const QXmppPresence &presence);

	/**
	 * Clears all cached presences.
	 */
	void clear();

Q_SIGNALS:
	/**
	 * Notifies about changed presences
	 */
	void presenceChanged(PresenceCache::ChangeType type, const QString &jid, const QString &resource);
	void presencesCleared();

private:
	constexpr qint8 availabilityPriority(QXmppPresence::AvailableStatusType type);
	bool presenceMoreImportant(const QXmppPresence &a, const QXmppPresence &b);

	QMap<QString, QMap<QString, QXmppPresence>> m_presences;

	static PresenceCache *s_instance;
};

class UserPresenceWatcher : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
	Q_PROPERTY(QString resource READ resource WRITE setResource NOTIFY resourceChanged)
	Q_PROPERTY(Presence::Availability availability READ availability NOTIFY presencePropertiesChanged)
	Q_PROPERTY(QString availabilityIcon READ availabilityIcon NOTIFY presencePropertiesChanged)
	Q_PROPERTY(QString availabilityText READ availabilityText NOTIFY presencePropertiesChanged)
	Q_PROPERTY(QColor availabilityColor READ availabilityColor NOTIFY presencePropertiesChanged)
	Q_PROPERTY(QString statusText READ statusText NOTIFY presencePropertiesChanged)

public:
	explicit UserPresenceWatcher(QObject *parent = nullptr);

	QString jid() const;
	void setJid(const QString &jid);

	QString resource() const;
	bool setResource(const QString &resource, bool autoPicked = false);

	Presence::Availability availability() const;
	QString availabilityIcon() const;
	QString availabilityText() const;
	QColor availabilityColor() const;
	QString statusText() const;

	Q_SIGNAL void jidChanged();
	Q_SIGNAL void resourceChanged();
	Q_SIGNAL void presencePropertiesChanged();

private:
	Q_SLOT void handlePresenceChanged(PresenceCache::ChangeType type,
		const QString &jid,
		const QString &resource);
	Q_SLOT void handlePresencesCleared();

	bool autoPickResource();

	QString m_jid;
	QString m_resource;
	bool m_resourceAutoPicked;
};

class UserResourcesWatcher : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
	Q_PROPERTY(int resourcesCount READ resourcesCount NOTIFY resourcesCountChanged)

public:
	explicit UserResourcesWatcher(QObject *parent = nullptr);

	QString jid() const;
	void setJid(const QString &jid);

	int resourcesCount();

	Q_SIGNAL void jidChanged();
	Q_SIGNAL void resourcesCountChanged();

private:
	QString m_jid;
};

// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>
// QXmpp
#include <QXmppVCardIq.h>

class Connection;
class QXmppVCardIq;
class VCardController;

class VCardModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(Connection *connection MEMBER m_connection WRITE setConnection)
    Q_PROPERTY(VCardController *vCardController MEMBER m_vCardController WRITE setVCardController)
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(bool unsetEntriesProcessed MEMBER m_unsetEntriesProcessed NOTIFY unsetEntriesProcessedChanged)

public:
    enum Roles {
        Key,
        Value,
        UriScheme,
    };

    struct Item {
        QString name;
        std::function<QString(const QXmppVCardIq *)> value;
        std::function<void(QXmppVCardIq *, const QString &)> setValue;
        QString uriScheme = {};
    };

    explicit VCardModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    void setConnection(Connection *connection);
    void setVCardController(VCardController *vCardController);

    QString jid() const;
    void setJid(const QString &jid);
    Q_SIGNAL void jidChanged();

    Q_SIGNAL void unsetEntriesProcessedChanged();

private:
    void requestVCard();
    void handleVCardReceived(const QXmppVCardIq &vCard);
    void generateEntries();

    Connection *m_connection;
    VCardController *m_vCardController;
    QString m_jid;

    bool m_unsetEntriesProcessed = false;
    QXmppVCardIq m_vCard;
    QList<Item> m_vCardMap;
};

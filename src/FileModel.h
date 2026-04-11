// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>
#include <QFutureWatcher>
// KDE
#include <KItemModels/KDescendantsProxyModel>
// Kaidan
#include "Message.h"

using Messages = QList<Message>;

class FileTreeModel;

class FileModel : public KDescendantsProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QString accountJid READ accountJid WRITE setAccountJid NOTIFY accountJidChanged)
    Q_PROPERTY(QString chatJid READ chatJid WRITE setChatJid NOTIFY chatJidChanged)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)

public:
    enum class Role {
        Message = Qt::UserRole,
        File,
    };
    Q_ENUM(Role)

    explicit FileModel(QObject *parent = nullptr);

    void setSourceModel(QAbstractItemModel *) override;

    QString accountJid() const;
    void setAccountJid(const QString &jid);
    Q_SIGNAL void accountJidChanged(const QString &jid);

    QString chatJid() const;
    void setChatJid(const QString &jid);
    Q_SIGNAL void chatJidChanged(const QString &jid);

    Messages files() const;
    void setFiles(const Messages &files);

    Q_INVOKABLE QModelIndex fileIndex(const QString &fileId) const;

    Q_INVOKABLE void loadFiles();

    Q_SIGNAL void rowCountChanged();

private:
    FileTreeModel *const m_sourceModel;
};

Q_DECLARE_METATYPE(FileModel::Role)

// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FileModel.h"

// Kaidan
#include "MessageDb.h"

class FileTreeModel : public QAbstractItemModel
{
    Q_OBJECT

    Q_PROPERTY(QString accountJid READ accountJid WRITE setAccountJid NOTIFY accountJidChanged)
    Q_PROPERTY(QString chatJid READ chatJid WRITE setChatJid NOTIFY chatJidChanged)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)

public:
    using Role = FileModel::Role;

    explicit FileTreeModel(QObject *parent = nullptr);
    ~FileTreeModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int columnCount(const QModelIndex &parent = {}) const override;

    int rowCount(const QModelIndex &parent = {}) const override;
    Q_SIGNAL void rowCountChanged();

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

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

private:
    void handleMessageUpdated(Message message);

    QString m_accountJid;
    QString m_chatJid;
    Messages m_files;
    QFutureWatcher<Messages> m_watcher;
};

FileTreeModel::FileTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    connect(MessageDb::instance(), &MessageDb::messageUpdated, this, &FileTreeModel::handleMessageUpdated);

    connect(&m_watcher, &QFutureWatcher<Messages>::finished, this, [this]() {
        setFiles(m_watcher.result());
    });
}

FileTreeModel::~FileTreeModel()
{
    m_watcher.cancel();
    m_watcher.waitForFinished();
}

QModelIndex FileTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return {};
    }

    // child node
    if (parent.isValid()) {
        return createIndex(row, column, static_cast<quintptr>(parent.row()));
    }

    // root node
    return createIndex(row, column, INT32_MAX);
}

QModelIndex FileTreeModel::parent(const QModelIndex &child) const
{
    // root node
    if (!child.isValid() || child.model() != this || child.internalId() == INT32_MAX) {
        return {};
    }

    // child node
    return createIndex(static_cast<int>(child.internalId()), 0, INT32_MAX);
}

int FileTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_ASSERT(parent == QModelIndex{} || parent.model() == this);
    return parent.isValid() && parent.column() != 0 ? 0 : 1;
}

int FileTreeModel::rowCount(const QModelIndex &parent) const
{
    if (hasIndex(parent.row(), parent.column(), parent.parent())) {
        // root node
        if (parent.internalId() == INT32_MAX) {
            const auto &msg = m_files[parent.row()];
            return msg.files.count();
        }

        // child node
        return 0;
    }

    // The invisible node
    return m_files.count();
}

QHash<int, QByteArray> FileTreeModel::roleNames() const
{
    static const auto roles = [this]() {
        auto roles = QAbstractItemModel::roleNames();
        roles.insert(static_cast<int>(Role::Message), QByteArrayLiteral("message"));
        roles.insert(static_cast<int>(Role::File), QByteArrayLiteral("file"));
        return roles;
    }();
    return roles;
}

QVariant FileTreeModel::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column(), index.parent())) {
        const auto msgIndex = index.internalId() == INT32_MAX ? index.row() : index.parent().row();
        Q_ASSERT(msgIndex != -1);
        const auto fileIndex = index.internalId() == INT32_MAX ? -1 : index.row();
        const auto &msg = m_files[msgIndex];
        const auto file = fileIndex != -1 ? std::optional<const File>(msg.files[fileIndex]) : std::nullopt;

        switch (role) {
        case Qt::DisplayRole: {
            if (file) {
                return file->httpSources.isEmpty() ? QString() : file->httpSources.constBegin()->url.toDisplayString();
            }

            return msg.body();
        }
        case static_cast<int>(Role::Message):
            return QVariant::fromValue(msg);
        case static_cast<int>(Role::File):
            return file ? QVariant::fromValue(*file) : QVariant{};
        }
    }

    return {};
}

QString FileTreeModel::accountJid() const
{
    return m_accountJid;
}

void FileTreeModel::setAccountJid(const QString &jid)
{
    if (m_accountJid != jid) {
        m_accountJid = jid;
        Q_EMIT accountJidChanged(m_accountJid);
    }
}

QString FileTreeModel::chatJid() const
{
    return m_chatJid;
}

void FileTreeModel::setChatJid(const QString &jid)
{
    if (m_chatJid != jid) {
        m_chatJid = jid;
        Q_EMIT chatJidChanged(m_chatJid);
    }
}

Messages FileTreeModel::files() const
{
    return m_files;
}

void FileTreeModel::setFiles(const Messages &files)
{
    if (m_files != files) {
        beginResetModel();
        m_files = files;
        endResetModel();

        Q_EMIT rowCountChanged();
    }
}

QModelIndex FileTreeModel::fileIndex(const QString &fileId) const
{
    const auto id = fileId.toLongLong();

    for (int i = 0; i < m_files.count(); ++i) {
        const auto &files = m_files[i].files;

        for (int j = 0; j < files.count(); ++j) {
            const auto &file = files[j];

            if (file.id == id) {
                return createIndex(j, 0, static_cast<quintptr>(i));
            }
        }
    }

    return {};
}

void FileTreeModel::loadFiles()
{
    Q_ASSERT(!m_accountJid.isEmpty());

    m_watcher.cancel();

    if (m_chatJid.isEmpty()) {
        m_watcher.setFuture(MessageDb::instance()->fetchFiles(m_accountJid));
    } else {
        m_watcher.setFuture(MessageDb::instance()->fetchFiles(m_accountJid, m_chatJid));
    }
}

void FileTreeModel::handleMessageUpdated(Message message)
{
    if (message.accountJid != m_accountJid || message.files.isEmpty()) {
        return;
    }

    const auto it = std::ranges::find_if(m_files, [&message](const Message &msg) {
        return msg.relevantId() == message.relevantId();
    });

    if (it == m_files.cend()) {
        return;
    }

    const int row = std::ranges::distance(m_files.cbegin(), it);
    const auto idx = index(row, 0);

    Q_EMIT layoutAboutToBeChanged({idx});
    *it = std::move(message);
    Q_EMIT layoutChanged({idx});
}

FileModel::FileModel(QObject *parent)
    : KDescendantsProxyModel(parent)
    , m_sourceModel(new FileTreeModel(this))
{
    connect(m_sourceModel, &FileTreeModel::accountJidChanged, this, &FileModel::accountJidChanged);
    connect(m_sourceModel, &FileTreeModel::chatJidChanged, this, &FileModel::chatJidChanged);
    connect(m_sourceModel, &FileTreeModel::rowCountChanged, this, &FileModel::rowCountChanged);

    KDescendantsProxyModel::setSourceModel(m_sourceModel);
}

void FileModel::setSourceModel(QAbstractItemModel *)
{
    Q_UNREACHABLE();
}

QString FileModel::accountJid() const
{
    return m_sourceModel->accountJid();
}

void FileModel::setAccountJid(const QString &jid)
{
    m_sourceModel->setAccountJid(jid);
}

QString FileModel::chatJid() const
{
    return m_sourceModel->chatJid();
}

void FileModel::setChatJid(const QString &jid)
{
    m_sourceModel->setChatJid(jid);
}

Messages FileModel::files() const
{
    return m_sourceModel->files();
}

void FileModel::setFiles(const Messages &files)
{
    m_sourceModel->setFiles(files);
}

QModelIndex FileModel::fileIndex(const QString &fileId) const
{
    const auto sourceIndex = m_sourceModel->fileIndex(fileId);

    if (sourceIndex.isValid()) {
        return mapFromSource(sourceIndex);
    }

    return {};
}

void FileModel::loadFiles()
{
    m_sourceModel->loadFiles();
}

#include "FileModel.moc"
#include "moc_FileModel.cpp"

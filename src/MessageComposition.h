// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QAbstractListModel>

#include "Message.h"

class FileSelectionModel;
class QFileDialog;
template <typename T>
class QFutureWatcher;

class MessageComposition : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString accountJid READ accountJid WRITE setAccountJid NOTIFY accountJidChanged)
	Q_PROPERTY(QString chatJid READ chatJid WRITE setChatJid NOTIFY chatJidChanged)
	Q_PROPERTY(QString replaceId READ replaceId WRITE setReplaceId NOTIFY replaceIdChanged)
	Q_PROPERTY(QString body READ body WRITE setBody NOTIFY bodyChanged)
	Q_PROPERTY(bool isSpoiler READ isSpoiler WRITE setSpoiler NOTIFY isSpoilerChanged)
	Q_PROPERTY(QString spoilerHint READ spoilerHint WRITE setSpoilerHint NOTIFY spoilerHintChanged)
	Q_PROPERTY(bool isDraft READ isDraft WRITE setIsDraft NOTIFY isDraftChanged)
	Q_PROPERTY(FileSelectionModel *fileSelectionModel MEMBER m_fileSelectionModel CONSTANT)

public:
	MessageComposition();
	~MessageComposition() override = default;

	[[nodiscard]] QString accountJid() const { return m_accountJid; }
	void setAccountJid(const QString &accountJid);
	[[nodiscard]] QString chatJid() const { return m_chatJid; }
	void setChatJid(const QString &chatJid);
	[[nodiscard]] QString replaceId() const { return m_replaceId; }
	void setReplaceId(const QString &replaceId);
	[[nodiscard]] QString body() const { return m_body; }
	void setBody(const QString &body);
	[[nodiscard]] bool isSpoiler() const { return m_spoiler; }
	void setSpoiler(bool spoiler);
	[[nodiscard]] QString spoilerHint() const { return m_spoilerHint; }
	void setSpoilerHint(const QString &spoilerHint);
	[[nodiscard]] bool isDraft() const { return m_isDraft; }
	void setIsDraft(bool isDraft);

	Q_INVOKABLE void send();
	void loadDraft();

	Q_SIGNAL void accountJidChanged();
	Q_SIGNAL void chatJidChanged();
	Q_SIGNAL void replaceIdChanged();
	Q_SIGNAL void bodyChanged();
	Q_SIGNAL void isSpoilerChanged();
	Q_SIGNAL void spoilerHintChanged();
	Q_SIGNAL void isDraftChanged();

private:
	void saveDraft();

	QString m_accountJid;
	QString m_chatJid;
	QString m_replaceId;
	QString m_body;
	bool m_spoiler = false;
	QString m_spoilerHint;
	bool m_isDraft = false;

	FileSelectionModel *m_fileSelectionModel;
};

class FileSelectionModel : public QAbstractListModel
{
	Q_OBJECT
public:
	enum Roles {
		Filename = Qt::UserRole + 1,
		Thumbnail,
		Description,
		FileSize,
	};

	explicit FileSelectionModel(QObject *parent = nullptr);
	~FileSelectionModel() override;

	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;
	[[nodiscard]] int rowCount(const QModelIndex &parent) const override;
	[[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

	Q_INVOKABLE void selectFile();
	Q_INVOKABLE void addFile(const QUrl &localFilePath);
	Q_INVOKABLE void removeFile(int index);
	Q_INVOKABLE void clear();
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	const QVector<File> &files() const;
	bool hasFiles() const {
		return !m_files.empty();
	}

	Q_SIGNAL void selectFileFinished();

private:
	void generateThumbnail(const File &file);

	QVector<File> m_files;
};

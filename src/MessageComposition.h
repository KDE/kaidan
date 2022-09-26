// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QAbstractListModel>

#include "Message.h"

class FileSelectionModel;
class QFileDialog;

class MessageComposition : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString account READ account WRITE setAccount NOTIFY accountChanged)
	Q_PROPERTY(QString to READ to WRITE setTo NOTIFY toChanged)
	Q_PROPERTY(QString body READ body WRITE setBody NOTIFY bodyChanged)
	Q_PROPERTY(bool isSpoiler READ isSpoiler WRITE setSpoiler NOTIFY isSpoilerChanged)
	Q_PROPERTY(QString spoilerHint READ spoilerHint WRITE setSpoilerHint NOTIFY spoilerHintChanged)
	Q_PROPERTY(FileSelectionModel *fileSelectionModel MEMBER m_fileSelectionModel CONSTANT)

public:
	MessageComposition();
	~MessageComposition() override = default;

	[[nodiscard]] QString account() const { return m_account; }
	void setAccount(const QString &account);
	[[nodiscard]] QString to() const { return m_to; }
	void setTo(const QString &to);
	[[nodiscard]] QString body() const { return m_body; }
	void setBody(const QString &body);
	[[nodiscard]] bool isSpoiler() const { return m_spoiler; }
	void setSpoiler(bool spoiler);
	[[nodiscard]] QString spoilerHint() const { return m_spoilerHint; }
	void setSpoilerHint(const QString &spoilerHint);

	Q_INVOKABLE void send();

	Q_SIGNAL void accountChanged();
	Q_SIGNAL void toChanged();
	Q_SIGNAL void bodyChanged();
	Q_SIGNAL void isSpoilerChanged();
	Q_SIGNAL void spoilerHintChanged();

private:
	QString m_account;
	QString m_to;
	QString m_body;
	bool m_spoiler = false;
	QString m_spoilerHint;

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

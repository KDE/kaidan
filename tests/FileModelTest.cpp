// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QImage>
#include <QFile>
#include <QTemporaryDir>
#include <QMimeDatabase>
#include <QMimeType>
#include <QtTest>

#include "../src/FileModel.h"
#include "../src/FileProxyModel.h"

class FileModelTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase() {
		qRegisterMetaType<QPersistentModelIndex>();
		qRegisterMetaType<QList<QPersistentModelIndex>>();
		qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>();

		QVERIFY(m_dir.isValid());
	}

	void cleanupTestCase() {
		const QFileInfo fileInfo(m_dir.path());

		m_dir.remove();

		QVERIFY(!fileInfo.exists());
	}

	void test_File_Proxy_Model()
	{
		FileModel model;
		FileProxyModel proxy;

		proxy.setSourceModel(&model);

		// Everything is empty.

		QCOMPARE(model.rowCount(), 0);
		QCOMPARE(proxy.rowCount(), 0);
		QCOMPARE(proxy.mode(), FileProxyModel::Mode::All);

		// Create files backed by local or only remote files.

		const auto localImage1 = createFile(QColor(Qt::red), true);
		const auto localImage2 = createFile(QColor(Qt::green), true);
		const auto localImage3 = createFile(QColor(Qt::blue), true);
		const auto image4 = createFile(QColor(Qt::blue), false);

		const auto localText1 = createFile(QStringLiteral("text1"), true);
		const auto localText2 = createFile(QStringLiteral("text2"), true);
		const auto localText3 = createFile(QStringLiteral("text3"), true);
		const auto text4 = createFile(QStringLiteral("text4"), false);

		QVERIFY(QFile::exists(localImage1.localFilePath));
		QVERIFY(QFile::exists(localImage2.localFilePath));
		QVERIFY(QFile::exists(localImage3.localFilePath));
		QVERIFY(!QFile::exists(image4.localFilePath));

		QVERIFY(QFile::exists(localText1.localFilePath));
		QVERIFY(QFile::exists(localText2.localFilePath));
		QVERIFY(QFile::exists(localText3.localFilePath));
		QVERIFY(!QFile::exists(text4.localFilePath));

		// Set up spies.

		QSignalSpy modelAboutToBeReset(&model, &FileModel::modelAboutToBeReset);
		QSignalSpy modelReset(&model, &FileModel::modelReset);

		QSignalSpy proxyAboutToBeReset(&proxy, &FileProxyModel::modelAboutToBeReset);
		QSignalSpy proxyReset(&proxy, &FileProxyModel::modelReset);
		QSignalSpy proxyLayoutAboutToBeChanged(&proxy, &FileProxyModel::layoutAboutToBeChanged);
		QSignalSpy proxyLayoutChanged(&proxy, &FileProxyModel::layoutChanged);
		QSignalSpy proxyCheckedCountChanged(&proxy, &FileProxyModel::checkedCountChanged);
		QSignalSpy proxyFilesDeleted(&proxy, &FileProxyModel::filesDeleted);

		const auto clearSpies = [&]() {
			modelAboutToBeReset.clear();
			modelReset.clear();
			proxyAboutToBeReset.clear();
			proxyReset.clear();
			proxyLayoutAboutToBeChanged.clear();
			proxyLayoutChanged.clear();
			proxyCheckedCountChanged.clear();
			proxyFilesDeleted.clear();
		};

		// Set files.

		model.setFiles({
						   localImage1,
						   localImage2,
						   image4,
						   localImage3,
						   localText1,
						   localText2,
						   text4,
						   localText3
					   });

		// Check emitted signals and counts.

		QVERIFY(modelAboutToBeReset.count() == 1 || modelAboutToBeReset.wait());
		QVERIFY(modelReset.count() == 1 || modelReset.wait());

		QVERIFY(proxyAboutToBeReset.count() == 1 || proxyAboutToBeReset.wait());
		QVERIFY(proxyReset.count() == 1 || proxyReset.wait());

		QCOMPARE(model.rowCount(), 8);
		// Not downloaded or manually deleted files are not counted.
		QCOMPARE(proxy.rowCount(), 6);

		clearSpies();

		// Check "Images" count.

		proxy.setMode(FileProxyModel::Mode::Images);

		QCOMPARE(proxy.rowCount(), 3);

		clearSpies();

		// Check "Videos" count.

		proxy.setMode(FileProxyModel::Mode::Videos);

		QCOMPARE(proxy.rowCount(), 0);

		clearSpies();

		// Check "Other" count.

		proxy.setMode(FileProxyModel::Mode::Other);

		QCOMPARE(proxy.rowCount(), 3);

		clearSpies();

		// Check "All" count.

		proxy.setMode(FileProxyModel::Mode::All);

		QCOMPARE(proxy.rowCount(), 6);

		clearSpies();

		const QModelIndex localImage1Index = proxy.index(0, 0);
		const QModelIndex localText2Index = proxy.index(5, 0);

		// Check some files for deletion.

		QVERIFY(proxy.setData(localImage1Index, Qt::Checked, Qt::CheckStateRole));
		QVERIFY(proxy.setData(localText2Index, Qt::Checked, Qt::CheckStateRole));

		QVERIFY(proxyCheckedCountChanged.count() == 2 || proxyCheckedCountChanged.wait());
		QVERIFY(proxyCheckedCountChanged.count() == 2);
		QCOMPARE(proxy.checkedCount(), 2);

		clearSpies();

		// Delete checked files.

		proxy.deleteChecked();

		QVERIFY(proxyCheckedCountChanged.count() == 1 || proxyCheckedCountChanged.wait());
		QVERIFY(proxyFilesDeleted.count() == 1 || proxyFilesDeleted.wait());

		QCOMPARE(proxy.checkedCount(), 0);
		// Not downloaded or manually deleted files are not counted.
		QCOMPARE(proxy.rowCount(), 4);

		clearSpies();
	}

private:
	QString filePath(const QColor &color) const {
		return m_dir.filePath(QStringLiteral("%1.png").arg(color.name().remove(QStringLiteral("#"))));
	}

	QString filePath(const QString &content) const {
		return m_dir.filePath(QStringLiteral("%1.txt").arg(qChecksum(content.toUtf8().constData(), content.length())));
	}

	QImage createImage(const QColor &color) const {
		const QFileInfo fileInfo(filePath(color));

		if (fileInfo.exists()) {
			return QImage(fileInfo.filePath());
		}

		QImage img(100, 100, QImage::Format_ARGB32_Premultiplied);
		img.fill(Qt::red);

		Q_ASSERT(img.save(fileInfo.filePath()));

		return img;
	}

	QByteArray createText(const QString &content) const {
		QFile file(filePath(content));

		if (file.exists()) {
			if (file.open(QIODevice::ReadOnly)) {
				return file.readAll();
			}

			Q_UNREACHABLE();
		}

		if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			if (file.write(content.toUtf8()) == -1) {
				file.remove();
				Q_UNREACHABLE();
			}
		}

		return content.toUtf8();
	}

	File createFile(const QColor &color, bool local) const {
		if (local) {
			createImage(color);
		}

		const QFileInfo fileInfo(filePath(color));
		File file;

		file.id = qChecksum(fileInfo.fileName().toUtf8().constData(), fileInfo.fileName().length());
		file.fileGroupId = file.id - 10;
		file.name = fileInfo.baseName();
		file.mimeType = m_db.mimeTypeForFile(fileInfo);
		file.size = fileInfo.size();
		file.lastModified = fileInfo.lastModified();
		file.localFilePath = local ? fileInfo.filePath() : QString();
		file.httpSources.append({
									file.id, QUrl(QStringLiteral("http://kaidan.im/colors/%1").arg(fileInfo.fileName()))
								});

		return file;
	}

	File createFile(const QString &content, bool local) const {
		if (local) {
			createText(content);
		}

		const QFileInfo fileInfo(filePath(content));
		File file;

		file.id = qChecksum(fileInfo.fileName().toUtf8().constData(), fileInfo.fileName().length());
		file.fileGroupId = file.id - 10;
		file.name = fileInfo.baseName();
		file.mimeType = m_db.mimeTypeForFile(fileInfo);
		file.size = fileInfo.size();
		file.lastModified = fileInfo.lastModified();
		file.localFilePath = local ? fileInfo.filePath() : QString();
		file.httpSources.append({
									file.id, QUrl(QStringLiteral("http://kaidan.im/texts/%1").arg(fileInfo.fileName()))
								});

		return file;
	}

private:
	QMimeDatabase m_db;
	QTemporaryDir m_dir;
};

QTEST_GUILESS_MAIN(FileModelTest)
#include "FileModelTest.moc"

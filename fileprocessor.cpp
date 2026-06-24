#include "fileprocessor.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>

FileProcessor::FileProcessor(QObject *parent) : QObject(parent) {}
FileProcessor::~FileProcessor() {}

void FileProcessor::processFiles(const QString &inDir, const QString &outDir, const QString &mask,
                                 bool deleteIn, bool overwrite, const QByteArray &key)
{
    m_isStopped = false;
    m_isPaused = false;

    QDir dir(inDir);
    if (!dir.exists()) {
        emit errorOccurred("Входная директория не существует.");
        emit finished();
        return;
    }

    QStringList filters;
    filters << mask;
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    if (files.isEmpty()) {
        emit progressChanged(0, "Файлы для обработки не найдены.");
        emit finished();
        return;
    }

    // Считаем общий объем для прогресс-бара
    qint64 totalSize = 0;
    for (const auto &f : files) totalSize += f.size();
    if (totalSize == 0) totalSize = 1; // Защита от деления на 0

    qint64 processedSize = 0;
    const qint64 bufferSize = 10 * 1024 * 1024; // 10 МБ чанк
    QByteArray buffer(bufferSize, 0);
    const char *k = key.constData();

    for (const auto &fileInfo : files) {
        if (m_isStopped) break;

        QString inPath = fileInfo.absoluteFilePath();
        QString outPath = resolveConflict(outDir, fileInfo.fileName(), overwrite);

        QFile inFile(inPath);
        QFile outFile(outPath);

        if (!inFile.open(QIODevice::ReadOnly)) {
            emit errorOccurred("Не удалось открыть: " + inPath);
            continue;
        }
        if (!outFile.open(QIODevice::WriteOnly)) {
            emit errorOccurred("Не удалось создать: " + outPath);
            continue;
        }

        emit progressChanged((processedSize * 100) / totalSize, "Обработка: " + fileInfo.fileName());

        while (!inFile.atEnd()) {
            checkPause();
            if (m_isStopped) break;

            qint64 bytesRead = inFile.read(buffer.data(), bufferSize);
            if (bytesRead <= 0) break;

            // Операция XOR с 8-байтным ключом
            char *data = buffer.data();
            for (qint64 i = 0; i < bytesRead; ++i) {
                data[i] ^= k[i & 7]; // i % 8
            }

            outFile.write(data, bytesRead);
            processedSize += bytesRead;

            // Обновляем прогресс
            int percent = (processedSize * 100) / totalSize;
            emit progressChanged(percent, QString("Обработка: %1 (%2%)").arg(fileInfo.fileName()).arg(percent));
        }

        inFile.close();
        outFile.close();

        if (deleteIn && !m_isStopped) {
            QFile::remove(inPath);
        }
    }

    emit progressChanged(100, m_isStopped ? "Прервано пользователем." : "Завершено успешно.");
    emit finished();
}

void FileProcessor::pauseProcessing() {
    m_isPaused = true;
}

void FileProcessor::resumeProcessing() {
    m_isPaused = false;
    m_waitCondition.wakeAll();
}

void FileProcessor::stopProcessing() {
    m_isStopped = true;
    m_isPaused = false; // Снимаем паузу, чтобы поток мог завершиться
    m_waitCondition.wakeAll();
}

void FileProcessor::checkPause() {
    QMutexLocker locker(&m_mutex);
    while (m_isPaused && !m_isStopped) {
        m_waitCondition.wait(&m_mutex);
    }
}

QString FileProcessor::resolveConflict(const QString &outDir, const QString &fileName, bool overwrite) {
    QString outPath = QDir(outDir).filePath(fileName);
    if (overwrite || !QFile::exists(outPath)) {
        return outPath;
    }

    // Добавление счетчика
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();
    QString ext = fi.suffix();
    int counter = 1;

    do {
        QString newName = QString("%1_%2.%3").arg(baseName).arg(counter).arg(ext);
        outPath = QDir(outDir).filePath(newName);
        counter++;
    } while (QFile::exists(outPath));

    return outPath;
}
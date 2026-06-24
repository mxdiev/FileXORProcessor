#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>

class FileProcessor : public QObject
{
    Q_OBJECT
public:
    explicit FileProcessor(QObject *parent = nullptr);
    ~FileProcessor();

public slots:
    void processFiles(const QString &inDir, const QString &outDir, const QString &mask,
                      bool deleteIn, bool overwrite, const QByteArray &key);
    void pauseProcessing();
    void resumeProcessing();
    void stopProcessing();

signals:
    void progressChanged(int percent, const QString &status);
    void finished();
    void errorOccurred(const QString &msg);

private:
    std::atomic<bool> m_isPaused{false};
    std::atomic<bool> m_isStopped{false};
    QMutex m_mutex;
    QWaitCondition m_waitCondition;

    void checkPause();
    QString resolveConflict(const QString &outDir, const QString &fileName, bool overwrite);
};

#endif // FILEPROCESSOR_H
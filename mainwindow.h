#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>

class FileProcessor;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QRadioButton;
class QSpinBox;
class QPushButton;
class QProgressBar;
class QLabel;
class QPlainTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onStartClicked();
    void onPauseClicked();
    void onResumeClicked();
    void onStopClicked();
    void onBrowseInDir();
    void onBrowseOutDir();
    void onTimerTick();
    
    void onProgressChanged(int percent, const QString &status);
    void onFinished();
    void onError(const QString &msg);

private:
    void setupUI();
    void startProcessing();
    void updateUIState(bool isProcessing);
    bool validateInputs();

    // UI Elements
    QLineEdit *m_editMask;
    QCheckBox *m_chkDelete;
    QLineEdit *m_editOutDir;
    QLineEdit *m_editInDir;
    QComboBox *m_cmbConflict;
    QRadioButton *m_radTimer;
    QRadioButton *m_radOneTime;
    QSpinBox *m_spinTimerPeriod;
    QLineEdit *m_editKey;
    
    QPushButton *m_btnStart;
    QPushButton *m_btnPause;
    QPushButton *m_btnResume;
    QPushButton *m_btnStop;
    
    QProgressBar *m_progressBar;
    QLabel *m_lblStatus;
    QPlainTextEdit *m_txtLog;

    // Threading
    QThread *m_workerThread;
    FileProcessor *m_worker;
    QTimer *m_pollTimer;
    bool m_isProcessing;
};

#endif // MAINWINDOW_H
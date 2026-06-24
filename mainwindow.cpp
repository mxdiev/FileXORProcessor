#include "mainwindow.h"
#include "fileprocessor.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QSpinBox>
#include <QRadioButton>
#include <QCloseEvent>
#include <QDir>
#include <QTimer>
#include <QThread>
#include <QMetaObject>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_isProcessing(false)
{
    setupUI();

    m_workerThread = new QThread(this);
    m_worker = new FileProcessor();
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &MainWindow::onFinished, this, &MainWindow::onFinished); // Self connect for UI update
    
    connect(m_worker, &FileProcessor::progressChanged, this, &MainWindow::onProgressChanged);
    connect(m_worker, &FileProcessor::finished, this, &MainWindow::onFinished);
    connect(m_worker, &FileProcessor::errorOccurred, this, &MainWindow::onError);

    m_workerThread->start();

    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);

    updateUIState(false);
}

MainWindow::~MainWindow()
{
    m_workerThread->quit();
    m_workerThread->wait();
}

void MainWindow::setupUI()
{
    setWindowTitle("Бинарный XOR Модификатор Файлов");
    resize(600, 500);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Settings Group
    QGroupBox *settingsBox = new QGroupBox("Настройки");
    QGridLayout *grid = new QGridLayout(settingsBox);

    grid->addWidget(new QLabel("Маска файлов:"), 0, 0);
    m_editMask = new QLineEdit("*.bin");
    grid->addWidget(m_editMask, 0, 1);

    grid->addWidget(new QLabel("Входная папка:"), 1, 0);
    m_editInDir = new QLineEdit();
    QPushButton *btnIn = new QPushButton("...");
    connect(btnIn, &QPushButton::clicked, this, &MainWindow::onBrowseInDir);
    QHBoxLayout *hIn = new QHBoxLayout();
    hIn->addWidget(m_editInDir); hIn->addWidget(btnIn);
    grid->addLayout(hIn, 1, 1);

    grid->addWidget(new QLabel("Выходная папка:"), 2, 0);
    m_editOutDir = new QLineEdit();
    QPushButton *btnOut = new QPushButton("...");
    connect(btnOut, &QPushButton::clicked, this, &MainWindow::onBrowseOutDir);
    QHBoxLayout *hOut = new QHBoxLayout();
    hOut->addWidget(m_editOutDir); hOut->addWidget(btnOut);
    grid->addLayout(hOut, 2, 1);

    grid->addWidget(new QLabel("При совпадении имен:"), 3, 0);
    m_cmbConflict = new QComboBox();
    m_cmbConflict->addItems({"Перезаписать", "Добавить счетчик (_1, _2)"});
    grid->addWidget(m_cmbConflict, 3, 1);

    grid->addWidget(new QLabel("XOR Ключ (16 hex символов):"), 4, 0);
    m_editKey = new QLineEdit("1234567890ABCDEF");
    m_editKey->setMaxLength(16);
    grid->addWidget(m_editKey, 4, 1);

    m_chkDelete = new QCheckBox("Удалять входные файлы после обработки");
    grid->addWidget(m_chkDelete, 5, 0, 1, 2);

    QHBoxLayout *modeLayout = new QHBoxLayout();
    m_radOneTime = new QRadioButton("Разовый запуск");
    m_radTimer = new QRadioButton("По таймеру (опрос)");
    m_radOneTime->setChecked(true);
    m_spinTimerPeriod = new QSpinBox();
    m_spinTimerPeriod->setRange(100, 3600000);
    m_spinTimerPeriod->setValue(5000);
    m_spinTimerPeriod->setSuffix(" мс");
    modeLayout->addWidget(m_radOneTime);
    modeLayout->addWidget(m_radTimer);
    modeLayout->addWidget(m_spinTimerPeriod);
    grid->addLayout(modeLayout, 6, 0, 1, 2);

    mainLayout->addWidget(settingsBox);

    // Controls
    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    m_btnStart = new QPushButton("Старт");
    m_btnPause = new QPushButton("Пауза");
    m_btnResume = new QPushButton("Продолжить");
    m_btnStop = new QPushButton("Стоп");
    
    ctrlLayout->addWidget(m_btnStart);
    ctrlLayout->addWidget(m_btnPause);
    ctrlLayout->addWidget(m_btnResume);
    ctrlLayout->addWidget(m_btnStop);
    mainLayout->addLayout(ctrlLayout);

    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(m_btnPause, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(m_btnResume, &QPushButton::clicked, this, &MainWindow::onResumeClicked);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);

    // Progress
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_lblStatus = new QLabel("Готов к работе.");
    mainLayout->addWidget(m_lblStatus);
    mainLayout->addWidget(m_progressBar);

    // Log
    m_txtLog = new QPlainTextEdit();
    m_txtLog->setReadOnly(true);
    mainLayout->addWidget(m_txtLog);
}

void MainWindow::onBrowseInDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выбор входной папки");
    if (!dir.isEmpty()) m_editInDir->setText(dir);
}

void MainWindow::onBrowseOutDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выбор выходной папки");
    if (!dir.isEmpty()) m_editOutDir->setText(dir);
}

bool MainWindow::validateInputs() {
    if (m_editInDir->text().isEmpty() || m_editOutDir->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите входную и выходную папки.");
        return false;
    }
    if (m_editMask->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите маску файлов.");
        return false;
    }
    QString keyStr = m_editKey->text();
    QByteArray keyBytes = keyStr.toLatin1();

    if (keyStr.length() != 16 || !std::all_of(keyBytes.begin(), keyBytes.end(), ::isxdigit)) {
        QMessageBox::warning(this, "Ошибка", "Ключ должен состоять ровно из 16 hex-символов (8 байт).");
        return false;
    }
    return true;
}

void MainWindow::onStartClicked()
{
    if (!validateInputs()) return;

    if (m_radTimer->isChecked()) {
        m_pollTimer->start(m_spinTimerPeriod->value());
        m_txtLog->appendPlainText("Таймер опроса запущен.");
        onTimerTick(); // Запускаем сразу первый раз
    } else {
        startProcessing();
    }
}

void MainWindow::onTimerTick()
{
    if (!m_isProcessing) {
        startProcessing();
    }
}

void MainWindow::startProcessing()
{
    if (m_isProcessing) return;

    QByteArray key = QByteArray::fromHex(m_editKey->text().toLatin1());
    bool overwrite = (m_cmbConflict->currentIndex() == 0);

    m_isProcessing = true;
    updateUIState(true);
    m_progressBar->setValue(0);

    // Используем QMetaObject::invokeMethod для безопасного вызова слота в другом потоке
    QMetaObject::invokeMethod(m_worker, "processFiles", Qt::QueuedConnection,
        Q_ARG(QString, m_editInDir->text()),
        Q_ARG(QString, m_editOutDir->text()),
        Q_ARG(QString, m_editMask->text()),
        Q_ARG(bool, m_chkDelete->isChecked()),
        Q_ARG(bool, overwrite),
        Q_ARG(QByteArray, key));
}

void MainWindow::onPauseClicked() {
    QMetaObject::invokeMethod(m_worker, "pauseProcessing", Qt::QueuedConnection);
    m_lblStatus->setText("Пауза...");
}

void MainWindow::onResumeClicked() {
    QMetaObject::invokeMethod(m_worker, "resumeProcessing", Qt::QueuedConnection);
    m_lblStatus->setText("Возобновление...");
}

void MainWindow::onStopClicked() {
    QMetaObject::invokeMethod(m_worker, "stopProcessing", Qt::QueuedConnection);
    m_pollTimer->stop();
    m_lblStatus->setText("Остановка...");
}

void MainWindow::onProgressChanged(int percent, const QString &status) {
    m_progressBar->setValue(percent);
    m_lblStatus->setText(status);
}

void MainWindow::onFinished() {
    m_isProcessing = false;
    updateUIState(false);
    if (!m_radTimer->isChecked()) {
        m_pollTimer->stop();
    }
}

void MainWindow::onError(const QString &msg) {
    m_txtLog->appendPlainText("Ошибка: " + msg);
}

void MainWindow::updateUIState(bool isProcessing) {
    m_btnStart->setEnabled(!isProcessing);
    m_btnPause->setEnabled(isProcessing);
    m_btnResume->setEnabled(isProcessing);
    m_btnStop->setEnabled(isProcessing);
    
    // Блокируем настройки во время работы
    m_editMask->setEnabled(!isProcessing);
    m_editInDir->setEnabled(!isProcessing);
    m_editOutDir->setEnabled(!isProcessing);
    m_editKey->setEnabled(!isProcessing);
    m_chkDelete->setEnabled(!isProcessing);
    m_cmbConflict->setEnabled(!isProcessing);
    m_radTimer->setEnabled(!isProcessing);
    m_radOneTime->setEnabled(!isProcessing);
    m_spinTimerPeriod->setEnabled(!isProcessing);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isProcessing) {
        QMessageBox::information(this, "Закрытие", "Идет обработка файлов. Останавливаем процесс...");
        QMetaObject::invokeMethod(m_worker, "stopProcessing", Qt::BlockingQueuedConnection);
    }
    
    m_pollTimer->stop();
    event->accept();
}
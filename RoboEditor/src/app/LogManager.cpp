#include "LogManager.h"

#include <QTextStream>

#include <QAction>
#include <QActionGroup>
#include <QDir>
#include <QDockWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QStandardPaths>

LogManager *LogManager::instance_ = nullptr;
QString     LogManager::logFileName_;
QString     LogManager::logFilePath_;
LogManager::LogManager(QObject *parent) : QObject(parent)
{
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);

    logFileName_ = "RoboEditor_" + QDate::currentDate().toString("yyyy-MM-dd") + "_Log.txt";
    logFilePath_ = logDir + "/" + logFileName_;
    openLogFile();
}

LogManager::~LogManager()
{
    closeLogFile();
}

// 싱글톤 초기화
void LogManager::initialize()
{
    if (!instance_) {
        instance_ = new LogManager();
        qDebug() << "[LogManager] Initialized";
    }
}

// 인스턴스 반환
LogManager *LogManager::instance()
{
    return instance_;
}

void LogManager::openLogFile()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    QDir dir(baseDir);
    if (!dir.exists())
        dir.mkpath(".");

    QString date = QDate::currentDate().toString("yyyy-MM-dd");
    logFileName_ = QString("RoboEditor_%1_Log.txt").arg(date);

    QString path = dir.filePath(logFileName_);
    logFilePath_ = path;

    file_ = new QFile(path, this);

    if (!file_->open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "[LogManager] Cannot open log file:" << path;
        delete file_;
        file_ = nullptr;
        return;
    }

    stream_ = new QTextStream(file_);

    // 구분선 추가
    (*stream_) << "\n";
    (*stream_) << "========================================\n";
    (*stream_) << "Program started at: "
               << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    (*stream_) << "========================================\n";
    stream_->flush();
}

void LogManager::closeLogFile()
{
    if (stream_) {
        stream_->flush();
        delete stream_;
        stream_ = nullptr;
    }

    if (file_) {
        file_->close();
        delete file_;
        file_ = nullptr;
    }
}
void LogManager::append(const QString &line)
{
    if (!instance_) {
        // 초기화 안된 경우 qDebug로 출력
        qDebug() << "[LogManager NOT INITIALIZED]" << line;
        return;
    }

    QString ts   = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString full = QString("[%1] %2").arg(ts, line);

    emit instance_->logAppended(full);

    // 파일에만 기록
    if (instance_->stream_) {
        (*instance_->stream_) << full << "\n";
        instance_->stream_->flush();
    }

    // File
    if (instance_->stream_) {
        (*instance_->stream_) << full << "\n";
        instance_->stream_->flush();
    }
}

void LogManager::destroy()
{
    if (instance_) {
        delete instance_;
        instance_ = nullptr;
    }
}

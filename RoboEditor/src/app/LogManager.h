#ifndef LOGMANAGER_H
#define LOGMANAGER_H
#pragma once
#include <QAction>
#include <QActionGroup>
#include <QDockWidget>
#include <QMainWindow>
#include <QObject>
#include <QPlainTextEdit>
class QMainWindow;
class QPlainTextEdit;
class QDockWidget;
class QAction;
class QActionGroup;

class LogManager : public QObject
{
    Q_OBJECT

      public:
        static void        initialize();  // 파라미터 제거
        static LogManager *instance();
        static void        destroy();
        static void        append(const QString &line);

        QString getLogFileName() const
        {
            return logFileName_;
        }
        QString getLogFilePath() const
        {
            return logFilePath_;
        }

      signals:
        void logAppended(const QString &line);

      private:
        explicit LogManager(QObject *parent = nullptr);  // 변경
        ~LogManager();

        void openLogFile();
        void closeLogFile();

        static LogManager *instance_;
        // 파일 관련만 유지
        QFile       *file_   = nullptr;
        QTextStream *stream_ = nullptr;
        QString      logFileName_;
        QString      logFilePath_;
};

#endif  // LOGMANAGER_H

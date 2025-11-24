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
        explicit LogManager(QMainWindow * mw,
                            QAction * actVisible,
                            QActionGroup * posGroup,
                            QAction * showLogParent);
        ~LogManager();
        static LogManager *instance_;
        static LogManager *instance();
        static void        initialize(
                QMainWindow * mw, QAction * vis, QActionGroup * grp, QAction * parent);
        static void append(const QString &line);
        static void destroy();

        QPlainTextEdit *view() const
        {
            return log_;
        }
        QDockWidget *dock() const
        {
            return dock_;
        }

        // public API
        void placeBottom();
        void placeRight();
        void placeLeft();
        void placeTop();
        void placeFloat();
        void setVisible(bool on);
        void placeLogAsDock(Qt::DockWidgetArea area, bool floating = false);

      private:
        QMainWindow    *mw_;
        QPlainTextEdit *log_          = nullptr;
        QDockWidget    *dock_         = nullptr;
        QAction        *actVisible_   = nullptr;
        QActionGroup   *posGroup_     = nullptr;
        QAction        *parentAction_ = nullptr;

        void buildDock();
        void wire();
        void syncChecks();  // 부모/자식 Visible 체크 동기화

        QFile       *file_   = nullptr;
        QTextStream *stream_ = nullptr;
        void         openLogFile();
        void         closeLogFile();
        void         connectSignals();
};

#endif  // LOGMANAGER_H

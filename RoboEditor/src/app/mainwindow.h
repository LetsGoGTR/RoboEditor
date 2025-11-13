#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once
#include <QDir>
#include <QFileDialog>
#include <QMainWindow>

#include <memory>

// Forward declarations
#include "BackupPage.h"
#include "ModifyPage.h"
#include "ShortcutManager.h"

class TopMenu;
class NavDock;
class CenterStack;
class LogManager;
class Center;

class MainWindow : public QMainWindow
{
    Q_OBJECT

      public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

      protected:
        void closeEvent(QCloseEvent *event) override;

      private:
        // lazy objects

        ModifyPage      *modifyPage;
        BackupPage      *backupPage;
        ShortcutManager *shortcutMgr;

        QWidget *applyPopup_  = nullptr;
        QWidget *backupPopup_ = nullptr;

        void ensureMenu();
        void ensureCenter();
        void ensureNav();
        void ensureLog();
        void wire();

        void saveSettings();
        void loadSettings();

        std::unique_ptr<TopMenu>     menu_;
        std::unique_ptr<CenterStack> center_;  // CenterStack → Center
        std::unique_ptr<NavDock>     nav_;
        std::unique_ptr<LogManager>  logm_;
};

#endif  // MAINWINDOW_H

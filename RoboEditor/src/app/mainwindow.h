#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once
#include <QDir>
#include <QFileDialog>
#include <QMainWindow>

#include <memory>

// Forward declarations
#include "BackupPage.h"
#include "ComparePage.h"
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
        static bool dark;
        static bool manualMode;

      public slots:
        void toggleTheme();
        void createNewDocument();
        void openFileFromMenu();
        void saveFileFromMenu();
        void saveAsFileFromMenu();
        void saveAllFromMenu();
        void closeFileFromMenu();
        void closeAllFromMenu();

        void undoFromMenu();
        void redoFromMenu();
        void cutFromMenu();
        void copyFromMenu();
        void pasteFromMenu();
        void selectAllFromMenu();

        void addCtrlFromMenu();
        void removeCtrlFromMenu();
        void modifyCtrlFromMenu();
        void refreshCtrlFromMenu();

      protected:
        void closeEvent(QCloseEvent * event) override;
        bool eventFilter(QObject * obj, QEvent * event) override;
        void applyStyleSheet();

      private:
        // lazy objects

        ModifyPage      *modifyPage;
        BackupPage      *backupPage;
        ShortcutManager *shortcutMgr;
        ComparePage     *comparePage = nullptr;

        QWidget *applyPopup_  = nullptr;
        QWidget *backupPopup_ = nullptr;

        void ensureMenu();
        void ensureCenter();
        void ensureNav();
        void ensureLog();
        void wire();

        void loadTheme(bool isDark);

        bool isDarkMode_ = false;

        std::unique_ptr<TopMenu>     menu_;
        std::unique_ptr<CenterStack> center_;  // CenterStack → Center
        std::unique_ptr<NavDock>     nav_;
        std::unique_ptr<LogManager>  logm_;
};

#endif  // MAINWINDOW_H

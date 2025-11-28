#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QMainWindow>
#include <QPointer>

#include <memory>

// Forward declarations
#include "BackupPage.h"
#include "ComparePage.h"
#include "ModifyPage.h"
#include "ShortcutManager.h"
#include "TopMenu.h"

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
        static bool        dark;
        static bool        manualMode;
        ControllerManager *getControllerManager();
        void               updateLogView();

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
        void refreshCtrlFromMenu();
        void populateModifyMenu(QMenu * menu);
        void populateRemoveMenu(QMenu * menu);

        void setFullScreen(bool enable);
        void setMaximize(bool enable);

        void applyFromMenu();
        void backupFromMenu();
        void compareFileFromMenu();
        void compareFolderFromMenu();

        void showUserGuideFromMenu();
        void showShortcutsFromMenu();
        void showSystemInfoFromMenu();
        void showAboutFromMenu();

        void onLogToggled(bool visible);
        void onLogPositionChanged(Qt::DockWidgetArea area, bool isPanel);

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

        bool isFullScreen_ = false;
        bool isDarkMode_   = false;
        bool m_logVisible  = true;
        bool m_logIsPanel  = true;

        Qt::DockWidgetArea    m_logArea = Qt::BottomDockWidgetArea;
        QPointer<QDockWidget> m_logDock;

        std::unique_ptr<TopMenu>     menu_;
        std::unique_ptr<CenterStack> center_;  // CenterStack → Center
        std::unique_ptr<NavDock>     nav_;
};

#endif  // MAINWINDOW_H

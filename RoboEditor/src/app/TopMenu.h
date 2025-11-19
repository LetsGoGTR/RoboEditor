#ifndef TOPMENU_H
#define TOPMENU_H
#pragma once
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QObject>

#include "PasswordManager.h"

class MainWindow;
class QMenu;
class QAction;
class QActionGroup;

class TopMenu : public QObject
{
    Q_OBJECT
      public:
        explicit TopMenu(MainWindow * mw);
        QMenu *viewMenu() const
        {
            return view_;
        }
        QAction *logVisibleAction() const
        {
            return actToggleLog_;
        }
        QActionGroup *logPosGroup() const
        {
            return posGroup_;
        }
        QAction *showLogParentAction() const
        {
            return showLogMenu_->menuAction();
        }

      private:
        MainWindow *mw_;
        QMenu      *file_ = nullptr, *edit_ = nullptr, *view_ = nullptr, *help_ = nullptr,
              *ctrl_ = nullptr, *tools_ = nullptr, *showLogMenu_ = nullptr;

        //File Action
        QAction *actNewFile_    = nullptr;
        QAction *actOpenFile_   = nullptr;
        QAction *actSaveFile_   = nullptr;
        QAction *actSaveAsFile_ = nullptr;
        QAction *actSaveAll_    = nullptr;
        QAction *actCloseFile_  = nullptr;
        QAction *actCloseAll_   = nullptr;
        QAction *actExit_       = nullptr;

        // Edit Actions
        QAction *actUndo_      = nullptr;
        QAction *actRedo_      = nullptr;
        QAction *actCut_       = nullptr;
        QAction *actCopy_      = nullptr;
        QAction *actPaste_     = nullptr;
        QAction *actSelectAll_ = nullptr;

        // Controller Actions
        QAction *actAddController_    = nullptr;
        QAction *actModifyController_ = nullptr;
        QAction *actRemoveController_ = nullptr;
        QAction *actRefreshList_      = nullptr;

        // View Actions
        QAction *actToggleLog_     = nullptr;
        QAction *actToggleToolbar_ = nullptr;
        QAction *actFullScreen_    = nullptr;
        QAction *actMaximize_      = nullptr;
        QAction *actResetLayout_   = nullptr;

        // Tools Actions
        QAction *actCompareFile_   = nullptr;
        QAction *actCompareFolder_ = nullptr;
        QAction *actBackup_        = nullptr;
        QAction *actApply_         = nullptr;
        QAction *actChangePswd_    = nullptr;

        // Help Actions
        QAction *actDocumentation_ = nullptr;
        QAction *actAbout_         = nullptr;

        QAction      *themeToggle_ = nullptr;
        QActionGroup *posGroup_    = nullptr;  // Bottom/Right/Left/Top/Float/Embedded
        void          build();

      private slots:
        void onChangePasswordTriggered();
};

#endif  // TOPMENU_H

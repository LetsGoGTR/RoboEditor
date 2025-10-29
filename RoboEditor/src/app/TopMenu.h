#ifndef TOPMENU_H
#define TOPMENU_H
#pragma once
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QObject>
class QMainWindow;
class QMenu;
class QAction;
class QActionGroup;

class TopMenu : public QObject
{
    Q_OBJECT
      public:
        explicit TopMenu(QMainWindow * mw);
        QMenu *viewMenu() const
        {
            return view_;
        }
        QAction *logVisibleAction() const
        {
            return actLogVisible_;
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
        QMainWindow *mw_;
        QMenu       *file_ = nullptr, *edit_ = nullptr, *view_ = nullptr, *help_ = nullptr;

        QAction *newFile_    = nullptr;
        QAction *openFile_   = nullptr;
        QAction *openFolder_ = nullptr;

        QMenu        *showLogMenu_   = nullptr;
        QAction      *actToggle_     = nullptr;  // Ctrl+L
        QAction      *actLogVisible_ = nullptr;  // Visible
        QActionGroup *posGroup_      = nullptr;  // Bottom/Right/Left/Top/Float/Embedded
        void          build();
};

#endif  // TOPMENU_H

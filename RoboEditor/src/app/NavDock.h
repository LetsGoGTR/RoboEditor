#ifndef NAVDOCK_H
#define NAVDOCK_H
#pragma once

#include <QToolBar>

#include <QAction>

class NavDock : public QToolBar
{
    Q_OBJECT
      public:
        explicit NavDock(QWidget *parent = nullptr);

      signals:
        void clickCompare();
        void clickBackup();
        void clickApply();
        void clickModify();
        void clickOpenFile();
      private slots:
        void onRegisterClicked();

      private:
        void build();
};

#endif  // NAVDOCK_H

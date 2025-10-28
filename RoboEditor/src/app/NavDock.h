#ifndef NAVDOCK_H
#define NAVDOCK_H
#pragma once
#include <QObject>

class QMainWindow;
class QDockWidget;

class NavDock : public QObject {
    Q_OBJECT
  public:
    explicit NavDock(QMainWindow* mw);

  signals:
    void clickCompare();
    void clickBackup();
    void clickApply();
    void clickModify();

  private:
    void build();
    QMainWindow* mw_;
    QDockWidget* dock_;
};

#endif // NAVDOCK_H

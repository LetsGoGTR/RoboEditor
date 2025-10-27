#ifndef NAVDOCK_H
#define NAVDOCK_H
#pragma once
#include <QObject>
class QMainWindow; class QDockWidget;
class NavDock : public QObject {
    Q_OBJECT
public:
    explicit NavDock(QMainWindow* mw);
    QDockWidget* dock() const { return dock_; }

signals:
    void clickCompare();
    void clickBackup();
    void clickApply();
    void clickModify();

private:
    QMainWindow* mw_; QDockWidget* dock_=nullptr;
    void build();
};

#endif // NAVDOCK_H

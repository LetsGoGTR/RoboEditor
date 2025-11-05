#ifndef LOGMANAGER_H
#define LOGMANAGER_H
#pragma once
#include <QObject>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QDockWidget>
#include <QAction>
#include <QActionGroup>
class QMainWindow; class QPlainTextEdit; class QDockWidget; class QAction; class QActionGroup;

class LogManager : public QObject {
    Q_OBJECT
public:
    explicit LogManager(QMainWindow* mw, QAction* actVisible, QActionGroup* posGroup, QAction* showLogParent);
    QPlainTextEdit* view() const { return log_; }
    QDockWidget* dock() const { return dock_; }

    // public API
    void placeBottom(); void placeRight(); void placeLeft(); void placeTop();
    void placeFloat();
    void setVisible(bool on);
    void append(const QString& line);
    void placeLogAsDock(Qt::DockWidgetArea area, bool floating=false);

private:
    QMainWindow* mw_;
    QPlainTextEdit* log_=nullptr; QDockWidget* dock_=nullptr;
    QAction* actVisible_=nullptr; QActionGroup* posGroup_=nullptr; QAction* parentAction_=nullptr;

    void buildDock(); void wire();
    void syncChecks();  // 부모/자식 Visible 체크 동기화
    void connectSignals();
};

#endif // LOGMANAGER_H

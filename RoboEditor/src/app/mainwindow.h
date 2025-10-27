#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once
#include <QMainWindow>
#include <memory>
class TopMenu; class NavDock; class CenterStack; class LogManager;

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(QWidget* parent=nullptr);
    ~MainWindow();

  private:
    // lazy objects
    std::unique_ptr<TopMenu> menu_;
    std::unique_ptr<NavDock> nav_;
    std::unique_ptr<CenterStack> center_;
    std::unique_ptr<LogManager> logm_;

    void ensureCenter(); void ensureNav(); void ensureMenu(); void ensureLog();
    void wire();
};


#endif // MAINWINDOW_H

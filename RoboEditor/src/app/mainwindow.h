#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDir>
#include <QFileDialog>
#include <QMainWindow>

#include <memory>

// Forward declarations
class Center;
class LogManager;
class NavDock;
class TopMenu;

class MainWindow : public QMainWindow
{
    Q_OBJECT

      public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

      private:
        void ensureMenu();
        void ensureCenter();
        void ensureNav();
        void ensureLog();
        void wire();

        std::unique_ptr<TopMenu>    menu_;
        std::unique_ptr<Center>     center_;  // CenterStack → Center
        std::unique_ptr<NavDock>    nav_;
        std::unique_ptr<LogManager> logm_;
};

#endif  // MAINWINDOW_H

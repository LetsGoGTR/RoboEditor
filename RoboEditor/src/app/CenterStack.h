#ifndef CENTERSTACK_H
#define CENTERSTACK_H
#pragma once

#include <QObject>

class QMainWindow;
class QStackedWidget;
class ComparePage;
class BackupPage;

class CenterStack : public QObject {
    Q_OBJECT
  public:
    explicit CenterStack(QMainWindow* mw);

    void showCompare();
    void showBackup();

  signals:
    void compareRequested(const QString& leftPath,
                          const QString& rightPath);
    void backupRequested(const QString& target);

  private:
    QStackedWidget* stack_;
    ComparePage*    cmp_;
    BackupPage*     bkp_;
    int idxC_;
    int idxB_;
};

#endif // CENTERSTACK_H

#ifndef BACKUPPAGE_H
#define BACKUPPAGE_H
#pragma once
#include <QWidget>
class QLineEdit;

class BackupPage : public QWidget {
    Q_OBJECT
public:
    explicit BackupPage(QWidget* parent=nullptr);
signals:
    void uiBackupClicked(const QString& target);
};

#endif // BACKUPPAGE_H

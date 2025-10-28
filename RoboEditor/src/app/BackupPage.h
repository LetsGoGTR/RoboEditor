#ifndef BACKUPPAGE_H
#define BACKUPPAGE_H
#pragma once

#include <QTableWidget>
#include <QTextEdit>
#include <QTreeView>

#include <QButtonGroup>
#include <QFileSystemModel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

class QLineEdit;

class BackupPage : public QWidget
{
    Q_OBJECT
      public:
        explicit BackupPage(QWidget *parent = nullptr);
        ~BackupPage() override;

      private:
        // Layouts
        QVBoxLayout *mainLayout;
        QHBoxLayout *contentLayout;
        QVBoxLayout *dirLayout;
        QVBoxLayout *workspaceLayout;
        QVBoxLayout *infoLayout;

        // File system
        QLabel           *dirTitle;
        QFileSystemModel *dirModel;
        QTreeView        *dirTree;
        QPushButton      *goUpButton;
        // Backup List
        QLabel       *backupTableTitle;
        QTableWidget *backupList;
        QPushButton  *backupDelete;
        QButtonGroup *radioGroup;

        // WorkDir Backup
        QLabel      *backupActionTitle;
        QLabel      *workDirInfo;
        QLabel      *sizeLabel;
        QLabel      *commentLabel;
        QTextEdit   *backupComment;
        QPushButton *backupCreate;

        // Function for Responsibility Principle
        void setupUi();
        void populateDummyData();

      signals:
        void uiBackupClicked(const QString &target);

      private slots:
        void onBackupCreateClicked();
        void onBackupDeleteClicked();
        void onRadioSelected(int id);
        void onDirectorySelected(const QModelIndex &index);  // 🔹 폴더 선택 슬롯 추가
};

#endif  // BACKUPPAGE_H

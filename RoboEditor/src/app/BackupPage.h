#ifndef BACKUPPAGE_H
#define BACKUPPAGE_H
#pragma once

#include <QTableWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeView>

#include <QButtonGroup>
#include <QFileSystemModel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
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
        // ===== Layouts =====
        QVBoxLayout *mainLayout;
        QHBoxLayout *contentLayout;
        QVBoxLayout *dirLayout;
        QVBoxLayout *workspaceLayout;
        QVBoxLayout *infoLayout;

        // ===== File system =====
        QLabel           *dirTitle;
        QFileSystemModel *dirModel;
        QTreeView        *dirTree;
        QToolButton      *upButton;
        QToolButton      *homeButton;
        QToolButton      *setHomeButton;
        QToolButton      *refreshButton;  // 🔹 새로고침 버튼 추가

        // ===== Backup List =====
        QLabel       *backupTableTitle;
        QTableWidget *backupList;
        QPushButton  *backupDelete;
        QButtonGroup *radioGroup;

        // ===== WorkDir Backup =====
        QLabel      *backupActionTitle;
        QLabel      *workDirInfo;
        QLabel      *sizeLabel;
        QLabel      *commentLabel;
        QTextEdit   *backupComment;
        QPushButton *backupCreate;

        // ===== Utility =====
        QSettings settings;  // Store App settings
        QString   customHomePath;

        // ===== Setup Functions =====
        void setupUi();
        void populateDummyData();
        void setupConnections();

      signals:
        void uiBackupClicked(const QString &target);

      private slots:
        // --- Directory control ---
        void onDirUpClicked();
        void onDirHomeClicked();
        void onSetHomeClicked();
        void onDirRefreshClicked();
        void onDirectorySelected(const QModelIndex &index);

        // --- Backup actions ---
        void onBackupCreateClicked();
        void onBackupDeleteClicked();
        void onRadioSelected(int id);
};

#endif  // BACKUPPAGE_H

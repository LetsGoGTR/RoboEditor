#ifndef BACKUPFORM_H
#define BACKUPFORM_H

#include <QTableWidget>
#include <QTextEdit>

#include <QButtonGroup>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

class BackupForm : public QWidget
{
    Q_OBJECT

      public:
        explicit BackupForm(QWidget *parent = nullptr);
        ~BackupForm() override;

      private:
        // Layouts
        QVBoxLayout *mainLayout;
        QHBoxLayout *contentLayout;
        QVBoxLayout *leftLayout;
        QVBoxLayout *rightLayout;

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

      private slots:
        void onBackupCreateClicked();
        void onBackupDeleteClicked();
        void onRadioSelected(int id);
    // void loadBackupList();
};

#endif  // BACKUPFORM_H

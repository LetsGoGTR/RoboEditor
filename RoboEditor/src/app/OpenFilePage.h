#ifndef OPENFILEPAGE_H
#define OPENFILEPAGE_H

#include <QTableWidget>
#include <QTreeWidget>

#include <QButtonGroup>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

class OpenFilePage : public QWidget
{
    Q_OBJECT
      public:
        explicit OpenFilePage(QWidget *parent = nullptr);

      private:
        // Layouts
        QVBoxLayout *mainLayout;
        QHBoxLayout *contentLayout;
        QVBoxLayout *robotLayout;
        QVBoxLayout *workspaceLayout;
        QVBoxLayout *infoLayout;

        // Robot Section
        QLabel       *robotTableTitle;
        QTableWidget *robotList;
        QButtonGroup *robotRadioGroup;

        // Workspace Section
        QLabel       *workspaceTableTitle;
        QTableWidget *workspaceList;
        QButtonGroup *workspaceRadioGroup;

        // Info Section (오른쪽 영역)
        QLabel      *detailTitle;
        QLabel      *detailDate;
        QLabel      *detailSize;
        QLabel      *commentLabel;
        QLabel      *commentValue;
        QLabel      *previewLabel;
        QTreeWidget *previewTree;
        QPushButton *loadButton;

        // 내부 함수
        void setupUi();
        void populateDummyData();

      signals:
        void uiOpenFileClicked(const QString &target);

      private slots:
        void onStorageFolderSelected(int id);
        void onRobotFolderSelected(int id);
        void onLoadClicked();
};

#endif  // OPENFILEPAGE_H

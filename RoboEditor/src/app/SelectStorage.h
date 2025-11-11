#ifndef SELECTSTORAGE_H
#define SELECTSTORAGE_H
#pragma once
#include <QTableView>

#include <QButtonGroup>
#include <QFileSystemModel>
#include <QListView>
#include <QRadioButton>
#include <QSplitter>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QWidget>

class selectTableWidget : public QWidget
{
    Q_OBJECT
      public:
        explicit selectTableWidget(QWidget *parent = nullptr);
        ~selectTableWidget() = default;

        QString getSelectedFolder() const;  // ✅ 선택된 폴더 경로 반환
        void    onRadioButtonChanged(int id);
        void    clearRadioSelection();

      signals:
        void folderSelected(const QString &folder);

      private slots:
        void onLeftListClicked(const QModelIndex &index);

      private:
        QListView          *leftList;
        QTableView         *rightTable;
        QStandardItemModel *rightModel;
        QFileSystemModel   *leftModel;
        QButtonGroup       *radioGroup;
        QString             rootPath;
        QString             currentPath;  // ✅ 현재 선택된 왼쪽 폴더 경로 저장

        void showSubFolders(const QString &path);
};

#endif  // SELECTSTORAGE_H

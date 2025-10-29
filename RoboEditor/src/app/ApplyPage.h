#pragma once
#include <QList>
#include <QWidget>

#include "SelectController.h"
#include "SelectStorage.h"

namespace Ui
{
    class ApplyPage;
}

class ApplyPage : public QWidget
{
    Q_OBJECT

      public:
        explicit ApplyPage(QWidget *parent = nullptr);
        ~ApplyPage();
        QString        selectedBackupDir;
        QList<QString> selectedControllerList;

      private:
        Ui::ApplyPage *ui;

        selectTableWidget *selectbackupWidget;
        selectcontroller  *selectcontrollerWidget;

        void showPasswordUI();
        void refreshList();
        void confirmSelection();
        void importAnyspace();

      signals:
        void uiApplyClicked(const QString &target);
};

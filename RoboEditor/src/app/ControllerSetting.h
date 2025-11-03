#ifndef CONTROLLERSETTING_H
#define CONTROLLERSETTING_H

#include <QDialog>

#include "ControllerManager.h"

namespace Ui
{
    class ControllerSettings;
}

class ControllerSetting : public QDialog
{
    Q_OBJECT

      public:
        explicit ControllerSetting(QWidget *parent = nullptr);
        ~ControllerSetting();

        ControllerInfo getControllerInfo();
        void           setControllerInfo(const ControllerInfo &info);

      private slots:
        void onAcceptBtn();
        void onRejectBtn();

      private:
        Ui::ControllerSettings *ui;
        ControllerInfo          m_info;
};

#endif  // CONTROLLERSETTING_H

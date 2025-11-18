#ifndef SELECTCONTROLLER_H
#define SELECTCONTROLLER_H
#pragma once
#include <QTableView>

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QDateTime>
#include <QEventLoop>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "../core/ControllerManager.h"

class selectcontroller : public QWidget
{
    Q_OBJECT
      public:
        selectcontroller(QWidget *parent = nullptr);
        ~selectcontroller();

        void             setupUI();
        void             getControllerState();
        void             updateTable();
        QVector<QString> getSelectedControllers() const;

        QStandardItemModel *model;
        QTableView         *table;

        QVector<ControllerInfo> controllerState;
        QVector<QCheckBox *>    checkBoxes;

      private:
        QMap<QString, bool> checkBoxStates_;

      private slots:
        void onControllerListChanged();
        void onControllerStateUpdated(const QString &serialNumber, bool isConnected, bool isRunning);

      signals:
        void controllerSelectionChanged();
};

#endif  // SELECTCONTROLLER_H

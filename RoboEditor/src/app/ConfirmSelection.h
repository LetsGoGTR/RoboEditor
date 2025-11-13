#ifndef CONFIRMSELECTION_H
#define CONFIRMSELECTION_H
#pragma once
#include <QDialog>
#include <QQueue>
#include <QPair>

namespace Ui
{
    class ConfirmSelection;
}

class ApplyPage;

class ConfirmSelection : public QDialog
{
    Q_OBJECT

      public:
        explicit ConfirmSelection(QWidget *parent = nullptr);
        ~ConfirmSelection();

        void setApplyPage(ApplyPage * page);

      signals:
        void applyRequested();

      public slots:
        void requestApply();

      private:
        Ui::ConfirmSelection *ui;
        ApplyPage            *applyPage = nullptr;  // ✅ 소문자로 시작 (카멜케이스)
};

#endif

#ifndef COMPAREPAGE_H
#define COMPAREPAGE_H
#pragma once
#include <QWidget>
class QLineEdit; class QPushButton;

class ComparePage : public QWidget {
    Q_OBJECT
public:
    explicit ComparePage(QWidget* parent=nullptr);
signals:
    void uiCompareClicked(const QString& left, const QString& right);
};

#endif // COMPAREPAGE_H

#include "ComparePage.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

ComparePage::ComparePage(QWidget* parent): QWidget(parent) {
    auto* left = new QLineEdit;  left->setPlaceholderText("Left path or ID");
    auto* right= new QLineEdit; right->setPlaceholderText("Right path or ID");
    auto* go   = new QPushButton("Compare (UI only)");
    connect(go, &QPushButton::clicked, this, [this, left, right]{
        emit uiCompareClicked(left->text(), right->text());
    });

    auto* ly = new QVBoxLayout(this);

    ly->addWidget(new QLabel("파일/프로젝트 비교 (Stub)"));
    ly->addWidget(left);
    ly->addWidget(right);
    ly->addWidget(go);
    ly->addStretch();
}

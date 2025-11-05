#include "NavDock.h"

#include <QAction>
#include <QIcon>
#include <QPushButton>
#include <QVBoxLayout>

#include "ControllerManager.h"
NavDock::NavDock(QWidget *parent) : QToolBar(parent)
{
    setMovable(false);
    setFloatable(false);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setIconSize(QSize(24, 24));
    build();
}
void NavDock::build()
{
    auto mk = [&](const QString &text, const QString &iconPath) {
        auto *act = new QAction(QIcon(iconPath), text, this);
        addAction(act);
        return act;
    };

    QAction *actRegister = mk("제어기 등록하기", ":/icons/compare.png");
    QAction *actCompare  = mk("비교하기", ":/icons/compare.png");
    QAction *actBackup   = mk("백업하기", ":/icons/backup.png");
    QAction *actApply    = mk("적용하기", ":/icons/apply.png");

    // 신호 연결
    connect(actRegister, &QAction::triggered, this, &NavDock::onRegisterClicked);
    connect(actCompare, &QAction::triggered, this, &NavDock::clickCompare);
    connect(actBackup, &QAction::triggered, this, &NavDock::clickBackup);
    connect(actApply, &QAction::triggered, this, &NavDock::clickApply);
}

void NavDock::onRegisterClicked()
{
    ControllerManager::instance()->registerController();
    qDebug() << "Controller registration dialog opened";
}

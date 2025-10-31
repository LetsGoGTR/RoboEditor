#include "NavDock.h"

#include <QIcon>

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

    QAction *actCompare = mk("비교하기", ":/icons/compare.png");
    QAction *actBackup  = mk("백업하기", ":/icons/backup.png");
    QAction *actApply   = mk("적용하기", ":/icons/apply.png");
    QAction *actModify  = mk("수정하기", ":/icons/modify.png");
    QAction *actOpen    = mk("확인하기", ":/icons/open.png");

    // 신호 연결
    connect(actCompare, &QAction::triggered, this, &NavDock::clickCompare);
    connect(actBackup, &QAction::triggered, this, &NavDock::clickBackup);
    connect(actApply, &QAction::triggered, this, &NavDock::clickApply);
    connect(actModify, &QAction::triggered, this, &NavDock::clickModify);
    connect(actOpen, &QAction::triggered, this, &NavDock::clickOpenFile);
}

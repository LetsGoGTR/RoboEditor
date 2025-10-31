#include "ShortcutManager.h"

#include <QWidget>

ShortcutManager::ShortcutManager(QObject *parent) : QObject{parent}
{
    setupShortcuts();
}

void ShortcutManager::setupShortcuts()
{
    // 파일 열기 (Ctrl + O)
    openAction = new QAction(tr("Open"), this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &ShortcutManager::openRequested);

    // 저장 (Ctrl + S)
    saveAction = new QAction(tr("Save"), this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &ShortcutManager::saveRequested);

    // 다른 이름으로 저장 (Ctrl + Shift + S)
    saveAsAction = new QAction(tr("saveAs"), this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &ShortcutManager::saveAsRequested);

    // 현재 파일 탭 닫기 (Ctrl + W)
    closeAction = new QAction(tr("Close Tab"), this);
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, &QAction::triggered, this, &ShortcutManager::closeRequested);

    // 프로그램 종료 (Ctrl + Q)
    quitAction = new QAction(tr("Quit"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &ShortcutManager::quitRequested);
}

void ShortcutManager::registerTo(QWidget *widget)
{
    widget->addAction(openAction);
    widget->addAction(saveAction);
    widget->addAction(saveAsAction);
    widget->addAction(closeAction);
    widget->addAction(quitAction);
}

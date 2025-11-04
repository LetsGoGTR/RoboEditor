#include "LogManager.h"
#include <QMainWindow>
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QAction>
#include <QActionGroup>
#include <QSignalBlocker>

LogManager::LogManager(QMainWindow* mw, QAction* vis, QActionGroup* grp, QAction* parent)
    : QObject(mw), mw_(mw), actVisible_(vis), posGroup_(grp), parentAction_(parent) {
    buildDock(); wire();
}

void LogManager::buildDock() {
    if (!log_) { log_ = new QPlainTextEdit; log_->setReadOnly(true); }

    mw_->setDockNestingEnabled(true);
    dock_ = new QDockWidget("Log", mw_);
    dock_->setObjectName("LogDock");
    dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    dock_->setWidget(log_);
    mw_->addDockWidget(Qt::BottomDockWidgetArea, dock_);
}

void LogManager::wire() {
    // 위치 액션 연결
    auto a = posGroup_->actions();
    connect(a[0], &QAction::triggered, this, &LogManager::placeBottom);
    connect(a[1], &QAction::triggered, this, &LogManager::placeRight);
    connect(a[2], &QAction::triggered, this, &LogManager::placeLeft);
    connect(a[3], &QAction::triggered, this, &LogManager::placeTop);
    connect(a[4], &QAction::triggered, this, &LogManager::placeFloat);
    // a[5] (Log → Center Bottom) 제거됨

    // Visible 토글
    connect(actVisible_, &QAction::toggled, this, [=](bool on){ setVisible(on); });

    // 부모(Show Log) 클릭 시에도 토글
    connect(parentAction_, &QAction::toggled, this, [=](bool on){ actVisible_->setChecked(on); });

    // 도크의 X 버튼/표시 변화와 동기화
    connect(dock_, &QDockWidget::visibilityChanged, this, [=](bool){ syncChecks(); });

    syncChecks();
}

void LogManager::syncChecks() {
  const bool vis = (dock_ && dock_->isVisible());
  QSignalBlocker b1(actVisible_), b2(parentAction_);
  if (actVisible_)    actVisible_->setChecked(vis);
  if (parentAction_)  parentAction_->setChecked(vis);
}

void LogManager::setVisible(bool on) {
    if (dock_) {
        dock_->setVisible(on);
        if (on) dock_->raise();
    }
    syncChecks();
}

void LogManager::placeLogAsDock(Qt::DockWidgetArea area, bool floating) {
    if (!log_) return;

    // 도크 준비/부착
    if (!dock_) {
        dock_ = new QDockWidget("Log", mw_);
        dock_->setObjectName("LogDock");
        dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
        dock_->setFeatures(QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetFloatable |
                           QDockWidget::DockWidgetClosable);
    }

    // log_를 도크에 재장착
    if (dock_->widget() != log_) {
        if (log_->parent()) log_->setParent(nullptr);
        dock_->setWidget(log_);
    }

    // 도킹/플로팅 적용
    mw_->addDockWidget(area, dock_);
    dock_->setFloating(floating);
    dock_->show();
    dock_->raise();

    setVisible(true);
    syncChecks();
}

void LogManager::placeBottom(){ mw_->addDockWidget(Qt::BottomDockWidgetArea, dock_); dock_->setFloating(false); setVisible(true); }
void LogManager::placeRight(){  mw_->addDockWidget(Qt::RightDockWidgetArea, dock_);  dock_->setFloating(false); setVisible(true); }
void LogManager::placeLeft(){   mw_->addDockWidget(Qt::LeftDockWidgetArea, dock_);   dock_->setFloating(false); setVisible(true); }
void LogManager::placeTop(){    mw_->addDockWidget(Qt::TopDockWidgetArea, dock_);    dock_->setFloating(false); setVisible(true); }
void LogManager::placeFloat(){  dock_->setFloating(true); dock_->show(); setVisible(true); }

void LogManager::append(const QString& line){
    if (!log_) {
        qDebug() << "[LogManager] log_ is null!";
        return;
    }
    if (log_) log_->appendPlainText(line);
}

#include "LogManager.h"
#include <QMainWindow>
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QAction>
#include <QActionGroup>
#include <QSplitter>
#include <QSignalBlocker>

LogManager::LogManager(QMainWindow* mw, QAction* vis, QActionGroup* grp, QAction* parent)
    : QObject(mw), mw_(mw), actVisible_(vis), posGroup_(grp), parentAction_(parent) {
    buildDock(); wire();
}

void LogManager::buildDock() {
    splitter_ = qobject_cast<QSplitter*>(mw_->centralWidget());
    if (!splitter_) {

        QWidget* oldCenter = mw_->centralWidget();
        splitter_ = new QSplitter(Qt::Vertical, mw_);

        if (oldCenter) {
            oldCenter->setParent(nullptr);
            splitter_->addWidget(oldCenter);
            splitter_->setStretchFactor(0, 1);
        }
        mw_->setCentralWidget(splitter_);
    }

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
    connect(a[5], &QAction::triggered, this, &LogManager::placeEmbedded);

    // Visible 토글
    connect(actVisible_, &QAction::toggled, this, [=](bool on){ setVisible(on); });

    // 부모(Show Log) 클릭 시에도 토글
    connect(parentAction_, &QAction::toggled, this, [=](bool on){ actVisible_->setChecked(on); });

    // 도크의 X 버튼/표시 변화와 동기화
    connect(dock_, &QDockWidget::visibilityChanged, this, [=](bool){ syncChecks(); });

    syncChecks();
}

void LogManager::syncChecks() {
  const bool vis = embedded_ ? (log_ && log_->isVisible()) : (dock_ && dock_->isVisible());
  QSignalBlocker b1(actVisible_), b2(parentAction_);
  if (actVisible_)    actVisible_->setChecked(vis);
  if (parentAction_)  parentAction_->setChecked(vis);
}

void LogManager::setVisible(bool on) {
    if (embedded_) {
        if (log_) log_->setVisible(on);
    } else {
        if (dock_) {
            dock_->setVisible(on);
            if (on) dock_->raise();
        }
    }
    syncChecks();
}

void LogManager::placeLogAsDock(Qt::DockWidgetArea area, bool floating) {
    if (!log_) return;

    // 1) 임베드면 컨테이너에서 정상 분리
    if (embedded_) {
        if (embeddedPanel_) {
            if (auto *lay = embeddedPanel_->layout())
                lay->removeWidget(log_);
            log_->setParent(nullptr);
            // 패널은 남겨둘 수 있지만(나중 임베드 재사용), 화면에 남지 않게 숨김
            embeddedPanel_->hide();
        } else if (auto* sp = qobject_cast<QSplitter*>(log_->parentWidget())) {
            int idx = sp->indexOf(log_);
            if (idx != -1) sp->replaceWidget(idx, new QWidget(sp)); // 자리메꿈
            log_->setParent(nullptr);
        }
        embedded_ = false;
    }

    // 2) 도크 준비/부착
    if (!dock_) {
        dock_ = new QDockWidget("Log", mw_);
        dock_->setObjectName("LogDock");
        dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
        dock_->setFeatures(QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetFloatable |
                           QDockWidget::DockWidgetClosable);
    }

    // 3) log_를 도크에 재장착 (가장 중요)
    if (dock_->widget() != log_) {
        if (log_->parent()) log_->setParent(nullptr);
        dock_->setWidget(log_);
    }

    // 4) 도킹/플로팅 적용
    mw_->addDockWidget(area, dock_);
    dock_->setFloating(floating);
    dock_->show();
    dock_->raise();

    setVisible(true);
    syncChecks();
}

void LogManager::placeBottom(){ mw_->addDockWidget(Qt::BottomDockWidgetArea, dock_); dock_->setFloating(false); embedded_=false; setVisible(true); }
void LogManager::placeRight(){  mw_->addDockWidget(Qt::RightDockWidgetArea, dock_);  dock_->setFloating(false); embedded_=false; setVisible(true); }
void LogManager::placeLeft(){   mw_->addDockWidget(Qt::LeftDockWidgetArea, dock_);   dock_->setFloating(false); embedded_=false; setVisible(true); }
void LogManager::placeTop(){    mw_->addDockWidget(Qt::TopDockWidgetArea, dock_);    dock_->setFloating(false); embedded_=false; setVisible(true); }
void LogManager::placeFloat(){  dock_->setFloating(true); dock_->show(); embedded_=false; setVisible(true); }

void LogManager::placeEmbedded() {
    if (!log_) return;

    // 1) 도크에서 뗀다 (도크는 유지)
    if (dock_ && dock_->widget() == log_) {
        dock_->setWidget(nullptr);
        dock_->hide(); // 빈 도크가 보이는 현상 방지
    }

    // 2) 스플리터 확보: 중앙이 스플리터가 아니면 기존 중앙을 상단으로 옮겨 세팅
    if (!splitter_) {
        if (auto* sp = qobject_cast<QSplitter*>(mw_->centralWidget())) {
            splitter_ = sp;
        } else {
            QWidget* old = mw_->centralWidget();           // 기존 중앙(보통 stacked)
            splitter_ = new QSplitter(Qt::Vertical, mw_);
            if (old) {
                old->setParent(nullptr);
                splitter_->addWidget(old);                   // 상단에 기존 중앙복귀
                splitter_->setStretchFactor(0, 1);
            }
            mw_->setCentralWidget(splitter_);
        }
    }

    // 3) 하단에 log_ 삽입 (중복 보호)
    if (!embeddedPanel_) {
        embeddedPanel_ = new QWidget;
        auto *v = new QVBoxLayout(embeddedPanel_); v->setContentsMargins(0,0,0,0); v->setSpacing(0);

        // 얇은 헤더(타이틀바 느낌)
        auto *header = new QWidget; header->setObjectName("LogHeader");
        auto *h = new QHBoxLayout(header); h->setContentsMargins(8,6,8,6);
        embeddedTitle_ = new QLabel("Log", header);
        QPushButton* popBtn = new QPushButton(tr("Pop out"));
        popBtn->setFlat(true);
        popBtn->setCursor(Qt::PointingHandCursor);
        h->addWidget(embeddedTitle_);
        h->addStretch();
        h->addWidget(popBtn);

        // 헤더 클릭 시 플로팅으로 전환
        connect(popBtn, &QPushButton::clicked, this, [=]{ placeLogAsDock(Qt::BottomDockWidgetArea, /*floating=*/true); });

        v->addWidget(header);

        // log_ 본체
        log_->setParent(embeddedPanel_);
        v->addWidget(log_);
    } else {
        // 이미 있다면 log_가 패널의 자식인지 확인
        if (log_->parentWidget() != embeddedPanel_) {
            log_->setParent(embeddedPanel_);
            auto *v = qobject_cast<QVBoxLayout*>(embeddedPanel_->layout());
            if (v) v->addWidget(log_);
        }
    }

    if (splitter_->indexOf(embeddedPanel_) == -1) {
        splitter_->addWidget(embeddedPanel_);
    }

    QList<int> sizes; sizes << 600 << 220;
    splitter_->setSizes(sizes);

    embedded_ = true;
    setVisible(true);
    syncChecks();
}

void LogManager::append(const QString& line){
    if (!log_) {
        qDebug() << "[LogManager] log_ is null!";
        return;
    }
    if (log_) log_->appendPlainText(line);
}

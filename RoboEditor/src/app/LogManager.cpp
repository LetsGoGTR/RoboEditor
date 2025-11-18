#include "LogManager.h"

#include <QTextStream>

#include <QAction>
#include <QActionGroup>
#include <QDir>
#include <QDockWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QStandardPaths>

LogManager *LogManager::instance_ = nullptr;

LogManager::LogManager(QMainWindow *mw, QAction *vis, QActionGroup *grp, QAction *parent) :
    QObject(mw),
    mw_(mw),
    actVisible_(vis),
    posGroup_(grp),
    parentAction_(parent)
{
    buildDock();
    wire();
    openLogFile();
}
LogManager::~LogManager()
{
    closeLogFile();
}

// 싱글톤 초기화
void LogManager::initialize(QMainWindow *mw, QAction *vis, QActionGroup *grp, QAction *parent)
{
    if (!instance_) {
        instance_ = new LogManager(mw, vis, grp, parent);
        qDebug() << "[LogManager] Singleton created:" << instance_;
    } else {
        qWarning() << "[LogManager] Already initialized!";
    }
}

// 인스턴스 반환
LogManager *LogManager::instance()
{
    return instance_;
}

void LogManager::buildDock()
{
    if (!log_) {
        log_ = new QPlainTextEdit;
        log_->setReadOnly(true);
    }

    mw_->setDockNestingEnabled(true);
    dock_ = new QDockWidget("Log", mw_);
    dock_->setObjectName("LogDock");
    dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable |
                       QDockWidget::DockWidgetClosable);
    dock_->setWidget(log_);
    mw_->addDockWidget(Qt::BottomDockWidgetArea, dock_);
}

void LogManager::wire()
{
    // 위치 액션 연결
    auto a = posGroup_->actions();
    connect(a[0], &QAction::triggered, this, &LogManager::placeBottom);
    connect(a[1], &QAction::triggered, this, &LogManager::placeRight);
    connect(a[2], &QAction::triggered, this, &LogManager::placeLeft);
    connect(a[3], &QAction::triggered, this, &LogManager::placeTop);
    connect(a[4], &QAction::triggered, this, &LogManager::placeFloat);
    // a[5] (Log → Center Bottom) 제거됨

    // Visible 토글
    connect(actVisible_, &QAction::toggled, this, [=](bool on) { setVisible(on); });

    // 부모(Show Log) 클릭 시에도 토글
    connect(parentAction_, &QAction::toggled, this, [=](bool on) { actVisible_->setChecked(on); });

    // 도크의 X 버튼/표시 변화와 동기화
    connect(dock_, &QDockWidget::visibilityChanged, this, [=](bool) { syncChecks(); });

    syncChecks();
}

void LogManager::syncChecks()
{
    const bool     vis = (dock_ && dock_->isVisible());
    QSignalBlocker b1(actVisible_), b2(parentAction_);
    if (actVisible_)
        actVisible_->setChecked(vis);
    if (parentAction_)
        parentAction_->setChecked(vis);
}

void LogManager::setVisible(bool on)
{
    if (dock_) {
        dock_->setVisible(on);
        if (on)
            dock_->raise();
    }
    syncChecks();
}

void LogManager::placeLogAsDock(Qt::DockWidgetArea area, bool floating)
{
    if (!log_)
        return;

    // 도크 준비/부착
    if (!dock_) {
        dock_ = new QDockWidget("Log", mw_);
        dock_->setObjectName("LogDock");
        dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
        dock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable |
                           QDockWidget::DockWidgetClosable);
    }

    // log_를 도크에 재장착
    if (dock_->widget() != log_) {
        if (log_->parent())
            log_->setParent(nullptr);
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

void LogManager::placeBottom()
{
    mw_->addDockWidget(Qt::BottomDockWidgetArea, dock_);
    dock_->setFloating(false);
    setVisible(true);
}
void LogManager::placeRight()
{
    mw_->addDockWidget(Qt::RightDockWidgetArea, dock_);
    dock_->setFloating(false);
    setVisible(true);
}
void LogManager::placeLeft()
{
    mw_->addDockWidget(Qt::LeftDockWidgetArea, dock_);
    dock_->setFloating(false);
    setVisible(true);
}
void LogManager::placeTop()
{
    mw_->addDockWidget(Qt::TopDockWidgetArea, dock_);
    dock_->setFloating(false);
    setVisible(true);
}
void LogManager::placeFloat()
{
    dock_->setFloating(true);
    dock_->show();
    setVisible(true);
}

void LogManager::openLogFile()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    QDir dir(baseDir);
    if (!dir.exists())
        dir.mkpath(".");

    QString date     = QDate::currentDate().toString("yyyy-MM-dd");
    QString fileName = QString("RoboEditor_%1_Log.txt").arg(date);

    QString path = dir.filePath(fileName);

    file_ = new QFile(path, this);

    if (!file_->open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "[LogManager] Cannot open log file:" << path;
        delete file_;
        file_ = nullptr;
        return;
    }

    stream_ = new QTextStream(file_);

    // 구분선 추가
    (*stream_) << "\n";
    (*stream_) << "========================================\n";
    (*stream_) << "Program started at: "
               << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    (*stream_) << "========================================\n";
    stream_->flush();
}

void LogManager::closeLogFile()
{
    if (stream_) {
        stream_->flush();
        delete stream_;
        stream_ = nullptr;
    }

    if (file_) {
        file_->close();
        delete file_;
        file_ = nullptr;
    }
}
void LogManager::append(const QString &line)
{
    if (!instance_) {
        // 초기화 안된 경우 qDebug로 출력
        qDebug() << "[LogManager NOT INITIALIZED]" << line;
        return;
    }

    QString ts   = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString full = QString("[%1] %2").arg(ts, line);

    // UI
    if (instance_->log_)
        instance_->log_->appendPlainText(full);

    // File
    if (instance_->stream_) {
        (*instance_->stream_) << full << "\n";
        instance_->stream_->flush();
    }
}
void LogManager::destroy()
{
    if (instance_) {
        delete instance_;
        instance_ = nullptr;
    }
}

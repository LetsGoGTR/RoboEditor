#include "ComparePage.h"

#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QTreeView>

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QToolButton>
#include <QStyle>
#include <QSplitter>
#include <QTextStream>
#include <QModelIndex>
#include <QTableWidget>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QAbstractItemView>

#include "DropTextEdit.h"

ComparePage::ComparePage(QWidget* parent) : QWidget(parent) {
    dock_ = buildDock();
    auto v = new QVBoxLayout(this);
    v->setContentsMargins(0,0,0,0);
    v->addWidget(dock_);
}

QWidget* ComparePage::buildDock() {
    auto w = new QWidget(this);
    auto v = new QVBoxLayout(w); v->setContentsMargins(0,0,0,0);

    // 헤더 + [X]
    auto header = new QWidget(w);
    auto h = new QHBoxLayout(header); h->setContentsMargins(6,4,6,4);
    auto title = new QLabel(tr("Compare"), header);
    auto btnClose = new QToolButton(header);
    btnClose->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    btnClose->setAutoRaise(true);
    h->addWidget(title); h->addStretch(); h->addWidget(btnClose);
    connect(btnClose, &QToolButton::clicked, this, &ComparePage::onCloseClicked);
    v->addWidget(header);

    // 내부 스플리터: (좌) 비교대상 텍스트(읽기 전용) | (우) diff 패널
    rightSplit_ = new QSplitter(Qt::Horizontal, w);
    rightText_  = new DropTextEdit(rightSplit_);
    rightText_->setReadOnly(true);
    diffPanel_  = buildDiffPanel(); // 기존 “테이블 + 상단 버튼” 유지
    rightSplit_->addWidget(rightText_);
    rightSplit_->addWidget(diffPanel_);
    rightSplit_->setStretchFactor(0, 1);
    rightSplit_->setStretchFactor(1, 0);

    v->addWidget(rightSplit_);
    return w;
}

QWidget* ComparePage::buildDiffPanel() {
    if (diffPanel_) return diffPanel_;
    diffPanel_ = new QWidget(this);
    auto v = new QVBoxLayout(diffPanel_);
    v->setContentsMargins(6,6,6,6);

    // 상단 정보 바: 좌/우 루트 및 통계
    auto infoBar = new QWidget(diffPanel_);
    auto ih = new QHBoxLayout(infoBar); ih->setContentsMargins(0,0,0,0);
    if (!fileInfoLabel_) fileInfoLabel_ = new QLabel("Left: - | Right: -", infoBar);
    if (!statLabel_)     statLabel_     = new QLabel("Diff: 0 changes", infoBar);
    ih->addWidget(fileInfoLabel_);
    ih->addStretch();
    ih->addWidget(statLabel_);
    v->addWidget(infoBar);

    // Diff 테이블
    if (!diffTable_) diffTable_ = new QTableWidget(diffPanel_);
    diffTable_->setColumnCount(4);
    diffTable_->setHorizontalHeaderLabels({"Key","Left","Right","State"});
    diffTable_->horizontalHeader()->setStretchLastSection(true);
    diffTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    diffTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    v->addWidget(diffTable_);

    return diffPanel_;
}


void ComparePage::onCloseClicked() { emit closed(); }

void ComparePage::setTargetPath(const QString& path) {
    targetPath_ = path;
    loadRightText(targetPath_);
}

void ComparePage::recalcDiff(const QString& leftText) {
    const QString rightText = rightText_ ? rightText_->toPlainText() : QString();

    // (임시) 빈 diff라도 테이블이 null이 아니도록 보장
    if (!diffTable_) buildDiffPanel(); // 방어적
    if (!diffTable_) return;

    // TODO: 이후 실제 diff 로직으로 교체
    QList<DiffRow> rows;
    if (!leftText.isEmpty() || !rightText.isEmpty()) {
        rows.push_back({"(preview)", QString::number(leftText.size()),
                        QString::number(rightText.size()), leftText == rightText ? "SAME" : "CHANGED"});
    }
    setDiffRows(rows);
}

// 내부 상태 업데이트용
void ComparePage::setRoots(const QString &leftRoot, const QString &rightRoot)
{
    currentLeftRoot_  = leftRoot;
    currentRightRoot_ = rightRoot;
    fileInfoLabel_->setText(
            QString("Left: %1 | Right: %2").arg(currentLeftRoot_, currentRightRoot_));
}

// 좌/우 본문 텍스트 채우기
void ComparePage::setFileContents(const QString &leftText, const QString &rightText)
{
    // leftText는 이제 ModifyPage가 가진다. 여기서는 우측만 갱신.
    Q_UNUSED(leftText);
    if (rightText_) rightText_->setPlainText(rightText);
}

// diff 테이블 갱신
void ComparePage::refreshDiffTable(const QList<DiffRow> &rows)
{
    diffTable_->setRowCount(rows.size());
    int changeCount = 0;

    for (int i = 0; i < rows.size(); i++) {
        const auto &r = rows[i];

        auto *itemKey   = new QTableWidgetItem(r.key);
        auto *itemLeft  = new QTableWidgetItem(r.origin);
        auto *itemRight = new QTableWidgetItem(r.target);
        auto *itemState = new QTableWidgetItem(r.state);

        if (r.state == "CHANGED") {
            itemKey->setBackground(Qt::yellow);
            itemLeft->setBackground(Qt::yellow);
            itemRight->setBackground(Qt::yellow);
            itemState->setBackground(Qt::yellow);
            changeCount++;
        } else if (r.state == "ADDED") {
            itemKey->setBackground(Qt::green);
            itemLeft->setBackground(Qt::green);
            itemRight->setBackground(Qt::green);
            itemState->setBackground(Qt::green);
            changeCount++;
        } else if (r.state == "REMOVED") {
            itemKey->setBackground(Qt::red);
            itemLeft->setBackground(Qt::red);
            itemRight->setBackground(Qt::red);
            itemState->setBackground(Qt::red);
            changeCount++;
        }

        diffTable_->setItem(i, 0, itemKey);
        diffTable_->setItem(i, 1, itemLeft);
        diffTable_->setItem(i, 2, itemRight);
        diffTable_->setItem(i, 3, itemState);
    }

    statLabel_->setText(QString("Diff: %1 changes").arg(changeCount));
}

void ComparePage::setDiffRows(const QList<DiffRow> &rows)
{
    refreshDiffTable(rows);
}

// ------------------------
// "API 자리 더미"
// ------------------------

// 더미 API: GET api/v1/folder?path=
void ComparePage::fetchFolderFromApiDummy(const QString &side, const QString &basePathHint)
{
    QString path =
            basePathHint.isEmpty() ? (side == "left" ? "/robot/A" : "/robot/B") : basePathHint;

    QStringList dummyFolders = {"cfg", "prog", "logs"};
    QStringList dummyFiles;

    if (side == "left") {
        dummyFiles = {"main.yaml", "task.srl", "io_map.yaml"};
    } else {
        dummyFiles = {"main.yaml", "task.srl", "io_map_v2.yaml"};
    }

    updateFolderListing(side, path, dummyFolders, dummyFiles);
}

// 더미 API: GET api/v1/file?path=... 2번 호출 + diff 결과
void ComparePage::fetchCompareFromApiDummy(const QString &leftFilePath,
                                           const QString &rightFilePath)
{
    Q_UNUSED(leftFilePath)
    Q_UNUSED(rightFilePath)

    QString leftContent = "paramA=10\n"
                          "paramB=20\n"
                          "paramC=OFF\n";

    QString rightContent = "paramA=10\n"
                           "paramB=22\n"
                           "paramC=OFF\n"
                           "paramD=EXTRA\n";

    setFileContents(leftContent, rightContent);

    QList<DiffRow> rows;
    rows.push_back({"paramA", "10", "10", ""});
    rows.push_back({"paramB", "20", "22", "CHANGED"});
    rows.push_back({"paramC", "OFF", "OFF", ""});
    rows.push_back({"paramD", "", "EXTRA", "ADDED"});

    setDiffRows(rows);
}

// ------------------------
// 버튼 핸들러
// ------------------------

void ComparePage::onCompareClicked()
{
    // QLineEdit에서 파일명 읽기
    const QString leftFileName  = leftFileSelect_  ? leftFileSelect_->text().trimmed()  : QString();
    const QString rightFileName = rightFileSelect_ ? rightFileSelect_->text().trimmed() : QString();

    if (leftFileName.isEmpty() || rightFileName.isEmpty()) {
        qWarning() << "[Compare] file name is empty" << leftFileName << rightFileName;
        return;
    }
    if (currentLeftRoot_.isEmpty() || currentRightRoot_.isEmpty()) {
        qWarning() << "[Compare] root path not set" << currentLeftRoot_ << currentRightRoot_;
        return;
    }

    // full path 조립
    const QString leftFullPath  = QDir(currentLeftRoot_).filePath(leftFileName);
    const QString rightFullPath = QDir(currentRightRoot_).filePath(rightFileName);

    // 로그/신호
    emit uiCompareClicked(leftFullPath, rightFullPath);

    // 좌/우 에디터 로딩
    loadFileIntoEditor(leftFullPath,  /*isLeft=*/true);
    loadFileIntoEditor(rightFullPath, /*isLeft=*/false);

    // 임시 diff
    fetchCompareFromApiDummy(leftFullPath, rightFullPath);
}


// 실제 로컬 디렉토리를 스캔해서 현재 콤보박스 갱신용으로 넘겨주는 헬퍼
void ComparePage::loadLocalFolder(const QString &side, const QString &path)
{
    // path 안의 파일 목록 가져오기
    QDir dir(path);

    // 안전장치: 폴더가 유효하지 않으면 그냥 리턴
    if (!dir.exists()) {
        qWarning() << "[ComparePage] Directory does not exist:" << path;
        return;
    }

    // 폴더 내부의 "파일들"만 뽑자 (지금은 1차 버전: 재귀 안 하고 바로 아래 것만)
    // 추후에는 YAML, SRL 등 필터링 가능
    QStringList   fileNames;
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    for (const QFileInfo &info : entries) {
        fileNames << info.fileName();
    }

    // 서브폴더 목록도 원하면 뽑을 수 있어 (지금은 UI 왼쪽 탭쪽에 붙일 예정)
    QStringList   folderNames;
    QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : subdirs) {
        folderNames << info.fileName();
    }

    // 기존 updateFolderListing() 재사용해서
    // - currentLeftRoot_ / currentRightRoot_ 갱신
    // - leftFileSelect_ / rightFileSelect_ 갱신
    updateFolderListing(side, path, folderNames, fileNames);

    refreshTreeView(side, path);
}

void ComparePage::loadRightText(const QString& path) {
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        const QString content = in.readAll();
        f.close();
        if (rightText_) rightText_->setPlainText(content);
    } else {
        if (rightText_) rightText_->setPlainText(tr("Failed to open: %1").arg(path));
    }
}

void ComparePage::loadFileIntoEditor(const QString &fullPath, bool isLeft)
{
    QFile f(fullPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream ts(&f);
    const QString content = ts.readAll();
    f.close();

    if (isLeft) {
        // 좌측은 ModifyPage 영역. 필요하면 시그널로 위임하도록 변경 가능.
        // 현재는 무시.
        return;
    } else {
        if (rightText_) rightText_->setPlainText(content);
    }
}
// ----- 선언만 있던 슬롯/헬퍼의 최소 구현 -----
void ComparePage::onOpenLeftFolderClicked()  { /* TODO: hook later */ }
void ComparePage::onOpenRightFolderClicked() { /* TODO: hook later */ }
void ComparePage::onLeftTreeDoubleClicked(const QModelIndex&)  { /* TODO */ }
void ComparePage::onRightTreeDoubleClicked(const QModelIndex&) { /* TODO */ }
void ComparePage::refreshTreeView(const QString& side, const QString& path) { Q_UNUSED(side); Q_UNUSED(path); }

void ComparePage::updateFolderListing(const QString& side,
                                      const QString& basePath,
                                      const QStringList& /*folders*/,
                                      const QStringList& /*files*/) {
    if (side == "left")  currentLeftRoot_  = basePath;
    else                 currentRightRoot_ = basePath;
    if (fileInfoLabel_)
        fileInfoLabel_->setText(QString("Left: %1 | Right: %2").arg(currentLeftRoot_, currentRightRoot_));
}

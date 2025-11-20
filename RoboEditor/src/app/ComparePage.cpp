#include "ComparePage.h"

#include <QTabWidget>
#include <QTextStream>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QUuid>
#include <QVBoxLayout>

#include <json/json.h>
#include <yaml-cpp/yaml.h>

#include "CodeEditor.h"
#include "DiffTablePanel.h"
#include "DropTextEdit.h"
#include "FileTypeHelper.h"
#include "FlowLayout.h"
#include "LogManager.h"

// Core services
#include "../core/services/DiffService.h"
#include "../core/services/WorkspaceService.h"

ComparePage::ComparePage(QWidget *parent) : QWidget(parent)
{
    this->setObjectName("ComparePageRoot");

    leftDiffHighlighter_  = new core::DiffHighlighter(this);
    rightDiffHighlighter_ = new core::DiffHighlighter(this);

    dock_  = buildDock();
    auto v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->addWidget(dock_);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

ComparePage::~ComparePage()
{
    cleanupTempFolders();
}

QWidget *ComparePage::buildDock()
{
    auto w = new QWidget(this);
    auto v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);

    rightSplit_ = new QSplitter(Qt::Horizontal, w);
    rightSplit_->setHandleWidth(2);

    compareTabWidget_ = new QTabWidget(rightSplit_);
    compareTabWidget_->setTabsClosable(true);

    connect(compareTabWidget_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        auto *widget = compareTabWidget_->widget(index);
        compareTabWidget_->removeTab(index);
        if (widget)
            widget->deleteLater();

        if (compareTabWidget_->count() == 0) {
            emit closed();
            return;
        }
        int curr = compareTabWidget_->currentIndex();
        if (curr >= 0) {
            rightText_   = qobject_cast<CodeEditor *>(compareTabWidget_->widget(curr));
            QString path = compareTabWidget_->tabToolTip(curr);
            if (!path.isEmpty()) {
                rightPath_ = path;
                emit rightFileChanged(path);
            }
        } else {
            rightText_ = nullptr;
        }
    });

    connect(compareTabWidget_, &QTabWidget::currentChanged, this, [this](int index) {
        if (index >= 0) {
            QWidget *tabPage = compareTabWidget_->widget(index);
            rightText_       = tabPage ? tabPage->findChild<CodeEditor *>() : nullptr;

            QString path = compareTabWidget_->tabToolTip(index);

            if (!path.isEmpty() && rightText_) {
                rightPath_ = path;
                emit rightFileChanged(path);

                if (leftText_) {
                    QString leftContent  = leftText_->toPlainText();
                    QString rightContent = rightText_->toPlainText();

                    qDebug() << "Tab changed - Left length:" << leftContent.length()
                             << "Right length:" << rightContent.length();

                    if (!leftContent.isEmpty() && !rightContent.isEmpty()) {
                        recalcDiff(leftContent, cachedLeftPath_);
                    }
                }
            }
        } else {
            rightText_ = nullptr;
        }
    });

    QWidget *rightPanel = buildRightPanel();

    rightSplit_->addWidget(compareTabWidget_);
    rightSplit_->addWidget(rightPanel);
    rightSplit_->setStretchFactor(0, 1);
    rightSplit_->setStretchFactor(1, 1);

    v->addWidget(rightSplit_);
    return w;
}

// 버튼 UI와 DiffTablePanel을 함께 생성하는 함수
QWidget *ComparePage::buildRightPanel()
{
    QWidget     *container = new QWidget(this);
    QVBoxLayout *layout    = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto topBar = new QWidget(container);
    topBar->setObjectName("topBar");
    auto topLayout = new FlowLayout(topBar, 8, 6, 6);

    auto btnSelectFile = new QPushButton(tr("Compare Files"), topBar);
    btnSelectFile->setFixedSize(100, 26);
    topLayout->addWidget(btnSelectFile);
    btnSelectFile->setObjectName("fileBtn");

    connect(btnSelectFile, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
                this,
                tr("Select file to compare"),
                "C:/backup",
                tr("All Files (*.*);;YAML Files (*.yaml *.yml);;Text Files (*.txt)"));

        if (!path.isEmpty()) {
            setRightFile(path);
        }
    });

    auto btnSelectFolder = new QPushButton(tr("Compare Folders"), topBar);
    btnSelectFolder->setFixedSize(120, 26);
    topLayout->addWidget(btnSelectFolder);
    btnSelectFolder->setObjectName("folderBtn");
    connect(btnSelectFolder, &QPushButton::clicked, this, [this]() {
        QFileDialog dialog(this, tr("Select folder or archive (compare)"), "C:/backup");
        dialog.setFileMode(QFileDialog::Directory);
        dialog.setOption(QFileDialog::ShowDirsOnly, false);
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setNameFilter(tr("Folders and Archives (*.zip *.tar *.tar.gz *.tgz)"));

        QListView *listView = dialog.findChild<QListView *>("listView");
        if (listView)
            listView->setSelectionMode(QAbstractItemView::SingleSelection);

        QString leftPath;
        if (dialog.exec() == QDialog::Accepted) {
            QStringList paths = dialog.selectedFiles();
            if (!paths.isEmpty())
                leftPath = paths.first();
        }
        if (leftPath.isEmpty())
            return;

        QFileDialog dialog2(this, tr("Select folder or archive (base)"), "C:/backup");
        dialog2.setFileMode(QFileDialog::Directory);
        dialog2.setOption(QFileDialog::ShowDirsOnly, false);
        dialog2.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog2.setNameFilter(tr("Folders and Archives (*.zip *.tar *.tar.gz *.tgz)"));

        QString rightPath;
        if (dialog2.exec() == QDialog::Accepted) {
            QStringList paths = dialog2.selectedFiles();
            if (!paths.isEmpty())
                rightPath = paths.first();
        }
        if (rightPath.isEmpty())
            return;

        performFolderDiff(leftPath, rightPath);
    });

    layout->addWidget(topBar);

    if (!diffPanel_) {
        diffPanel_ = new DiffTablePanel(this);
        connect(diffPanel_,
                &DiffTablePanel::fileDoubleClicked,
                this,
                &ComparePage::onFolderFileClicked);
    }
    layout->addWidget(diffPanel_);

    return container;
}

void ComparePage::onFolderFileClicked(const QString &relPath)
{
    // 폴더 루트 정보가 없으면 일단 ModifyPage 쪽으로만 넘김
    if (lastLeftFolderPath_.isEmpty() || lastRightFolderPath_.isEmpty()) {
        emit requestOpenFile(relPath);
        return;
    }

    // DiffTablePanel에서 넘겨주는 path는 "config/common/param/analog_io.yaml"
    // 같은 상대경로라고 가정하고, 루트에 붙여서 실제 파일 경로를 만든다.
    QString leftFile  = QDir(lastLeftFolderPath_).filePath(relPath);
    QString rightFile = QDir(lastRightFolderPath_).filePath(relPath);

    // 1) 왼쪽(Compare 기준) 파일을 ModifyPage에서 열기
    emit requestOpenFile(leftFile);

    // 2) 파일 비교 모드로 전환
    performDiff(leftFile, rightFile);
}

void ComparePage::setLeftEditor(CodeEditor *leftEditor)
{
    leftText_ = leftEditor;
    if (leftText_) {
        leftText_->setDiffHighlighter(leftDiffHighlighter_);
        cachedLeftPath_ = leftText_->lastLoadedPath();
        cachedLeftText_ = leftText_->toPlainText();
        if (!rightPath_.isEmpty()) {
            recalcDiff(cachedLeftText_, cachedLeftPath_);
        }
    }
}

void ComparePage::clearHighlights()
{
    if (leftText_) {
        leftText_->clearDiffHighlights();
        leftText_->setDiffHighlighter(nullptr);
    }
    if (rightText_) {
        rightText_->clearDiffHighlights();
        rightText_->setDiffHighlighter(nullptr);
    }
    if (leftDiffHighlighter_)
        leftDiffHighlighter_->clearLineStates();
    if (rightDiffHighlighter_)
        rightDiffHighlighter_->clearLineStates();
}

QString ComparePage::formatPathForCompare(const QString &fullPath) const
{
    if (fullPath.isEmpty())
        return "No file opened";
    QString displayPath = fullPath;
    if (displayPath.startsWith("C:/backup", Qt::CaseInsensitive))
        displayPath.remove(0, 9);
    else if (displayPath.startsWith("C:\\backup", Qt::CaseInsensitive))
        displayPath.remove(0, 9);
    if (displayPath.startsWith('/') || displayPath.startsWith('\\'))
        displayPath.remove(0, 1);
    displayPath.replace('/', " > ");
    displayPath.replace('\\', " > ");
    return displayPath;
}

//오른쪽 코드 에디터 만들고 targetPath 설정
void ComparePage::setRightFile(const QString &path)
{
    if (path.isEmpty() || !compareTabWidget_)
        return;

    rightPath_ = path;

    QString content = loadFileContent(path);
    rightText_      = createCompareTab(path, content);

    emit rightFileChanged(path);
}

// 새로운 compareTab 생성
CodeEditor *ComparePage::createCompareTab(const QString &path, const QString &content)
{
    QFileInfo fileInfo(path);
    QString   fileName     = fileInfo.fileName();
    QString   absolutePath = fileInfo.absoluteFilePath();

    // 기존 탭 제거
    // while (compareTabWidget_->count() > 0) {
    //     auto *widget = compareTabWidget_->widget(0);
    //     compareTabWidget_->removeTab(0);
    //     if (widget) widget->deleteLater();
    // }

    // 새 탭 생성
    QWidget     *tabPage    = new QWidget(compareTabWidget_);
    QVBoxLayout *pageLayout = new QVBoxLayout(tabPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    // 경로 레이블
    QLabel *pathLabel = new QLabel(tabPage);
    pathLabel->setObjectName("pathLabel");
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pathLabel->setText(formatPathForCompare(absolutePath));
    pageLayout->addWidget(pathLabel);

    // 에디터 생성
    auto *editor = new CodeEditor(tabPage);
    editor->setReadOnly(true);
    editor->setAccept(true);
    if (rightDiffHighlighter_) {
        editor->setDiffHighlighter(rightDiffHighlighter_);
    }
    editor->setPlainText(content);
    pageLayout->addWidget(editor);

    // 드롭 이벤트 연결
    connect(editor, &DropTextEdit::fileDropped, this, [this](const QString &droppedPath) {
        setRightFile(droppedPath);
    });

    // 탭 추가
    int index = compareTabWidget_->addTab(tabPage, fileName);
    compareTabWidget_->setTabToolTip(index, absolutePath);
    compareTabWidget_->setCurrentIndex(index);

    return editor;
}
QString ComparePage::loadFileContent(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return tr("Failed to open: %1").arg(path);
    }
    QTextStream in(&f);
    return in.readAll();
}
void ComparePage::setDiffRows(const QList<DiffRow> &rows)
{
    // 1. 에디터 하이라이터(CodeEditor) 업데이트 로직
    QMap<int, QString> leftLineStates;
    QMap<int, QString> rightLineStates;

    for (const DiffRow &row : rows) {
        // 상태가 SAME이 아닐 때만 하이라이팅
        if (row.state != "SAME") {
            if (row.leftLineNumber > 0) {
                leftLineStates[row.leftLineNumber] = row.state;
            }
            if (row.rightLineNumber > 0) {
                rightLineStates[row.rightLineNumber] = row.state;
            }
            // 에러 라인 하이라이팅 (line 공통 필드 사용)
            if (row.state == "ERROR" && row.line > 0) {
                leftLineStates[row.line]  = "ERROR";
                rightLineStates[row.line] = "ERROR";
            }
        }
    }

    // 좌측 에디터 하이라이터 적용
    if (leftDiffHighlighter_) {
        leftDiffHighlighter_->setLineStates(leftLineStates);
    }

    // 우측 에디터 하이라이터 적용
    if (rightDiffHighlighter_) {
        rightDiffHighlighter_->setLineStates(rightLineStates);
    }

    // 우측 에디터가 교체되었을 수 있으므로 다시 연결 확인
    if (rightText_ && rightText_->getDiffHighlighter() != rightDiffHighlighter_) {
        rightText_->setDiffHighlighter(rightDiffHighlighter_);
    }

    // 2. DiffTablePanel(테이블 UI) 업데이트 위임
    if (diffPanel_) {
        diffPanel_->setDiffRows(rows);
    }
}

//modiifyPage에서 왼쪽 에디터가 호출
void ComparePage::recalcDiff(const QString &leftText, const QString &leftPath)
{
    if (!compareTabWidget_ || compareTabWidget_->count() == 0)
        return;

    const QString rightText         = rightText_ ? rightText_->toPlainText() : QString();
    QString       effectiveLeftPath = leftPath;
    if (effectiveLeftPath.isEmpty() && leftText_) {
        effectiveLeftPath = leftText_->lastLoadedPath();
    }
    cachedLeftText_ = leftText;
    cachedLeftPath_ = effectiveLeftPath;

    if (!diffPanel_)
        return;

    if (rightPath_.isEmpty()) {
        diffPanel_->clearAll();
        clearHighlights();  // 경로 없으면 하이라이트도 제거
        return;
    }

    // 1. 파일 타입 감지
    auto lType = FileTypeHelper::detect(effectiveLeftPath);
    auto rType = FileTypeHelper::detect(rightPath_);

    if (!effectiveLeftPath.isEmpty() && !FileTypeHelper::isCompatible(lType, rType)) {
        const QString leftTypeName  = FileTypeHelper::typeName(lType);
        const QString rightTypeName = FileTypeHelper::typeName(rType);

        // 상단 타입 정보 박스에는 그대로 표시
        if (diffPanel_) {
            diffPanel_->setFileTypeInfo(leftTypeName, rightTypeName, false, tr("호환되지 않음"));
        }

        // 테이블에 "확장자 불일치" 한 줄 추가
        DiffRow row;
        row.line            = -1;
        row.leftLineNumber  = -1;
        row.rightLineNumber = -1;
        row.key             = tr("확장자 불일치");
        row.origin          = rightTypeName;  // 오른쪽 타입
        row.target          = leftTypeName;   // 왼쪽 타입
        row.state           = "MISMATCH";     // 하이라이트는 안 줄 상태값

        QList<DiffRow> rows;
        rows.append(row);

        // 에디터 하이라이트는 필요 없으니 먼저 지우고
        clearHighlights();
        // 테이블만 갱신
        setDiffRows(rows);

        return;
    }

    QString                 fileTypeStr = FileTypeHelper::typeName(rType);
    QPair<QString, QString> headers     = determineColumnHeaders(effectiveLeftPath, rightPath_);

    // UI 설정
    diffPanel_->setupFileDiffMode(fileTypeStr, headers.first, headers.second);

    // 2. Diff 수행
    services::ServiceResult result = services::DiffService::diff(rightText.toStdString(),
                                                                 leftText.toStdString(),
                                                                 rightPath_.toStdString(),
                                                                 effectiveLeftPath.toStdString());

    // 3. 에러 처리 (라인 번호 복구)
    if (!result.success) {
        QList<DiffRow> errRows;
        bool           handled = false;

        if (fileTypeStr == "yaml") {
            bool    errorFound = false;
            int     errorLine  = -1;
            QString errorDetail;

            // 좌측 파싱 시도
            try {
                YAML::Load(leftText.toStdString());
            } catch (const YAML::Exception &e) {
                errorLine   = e.mark.line + 1;
                errorFound  = true;
                errorDetail = QString::fromStdString(e.msg);
            }

            // 우측 파싱 시도
            if (!errorFound) {
                try {
                    YAML::Load(rightText.toStdString());
                } catch (const YAML::Exception &e) {
                    errorLine   = e.mark.line + 1;
                    errorFound  = true;
                    errorDetail = QString::fromStdString(e.msg);
                }
            }

            if (errorFound) {
                DiffRow r;
                r.line            = errorLine;  // 라인 번호 할당!
                r.leftLineNumber  = errorLine;  // 하이라이터를 위해 할당
                r.rightLineNumber = errorLine;
                r.state           = "ERROR";
                r.key             = (errorLine > 0) ? QString::number(errorLine) : "Parse Error";
                r.target          = "YAML 형식이 올바르지 않습니다.";
                r.origin          = errorDetail;  // 상세 에러는 우측에 표시

                errRows.append(r);
                setDiffRows(errRows);  // ComparePage::setDiffRows 호출
                handled = true;
            }
        }

        if (!handled) {
            DiffRow r;
            r.key    = "Error";
            r.target = QString::fromStdString(result.errorMessage);
            r.state  = "ERROR";
            errRows.append(r);
            setDiffRows(errRows);  // [수정] ComparePage::setDiffRows 호출
        }
        return;
    }

    // 4. 결과 변환 및 적용
    QList<DiffRow> diffRows = parseDiffResult(result.data, fileTypeStr);

    int addedCount = 0, removedCount = 0, changedCount = 0;
    for (const DiffRow &row : diffRows) {
        if (row.state == "ADDED")
            addedCount++;
        else if (row.state == "REMOVED")
            removedCount++;
        else if (row.state == "CHANGED")
            changedCount++;
    }

    // 빈 결과일 때 YAML 에러 재확인 (DiffYaml 특성 대응)
    if (diffRows.isEmpty() && fileTypeStr == "yaml") {
        bool errorFound = false;
        int  errorLine  = -1;

        try {
            YAML::Load(leftText.toStdString());
        } catch (const YAML::Exception &e) {
            errorLine  = e.mark.line + 1;
            errorFound = true;
        }

        if (!errorFound) {
            try {
                YAML::Load(rightText.toStdString());
            } catch (const YAML::Exception &e) {
                errorLine  = e.mark.line + 1;
                errorFound = true;
            }
        }

        if (errorFound) {
            DiffRow r;
            r.line            = errorLine;  // 라인 번호 할당
            r.leftLineNumber  = errorLine;
            r.rightLineNumber = errorLine;
            r.key             = (errorLine > 0) ? QString::number(errorLine) : "Parse Error";
            r.target          = "YAML 형식이 올바르지 않습니다.";
            r.state           = "ERROR";
            diffRows.append(r);
        }
    }

    setDiffRows(diffRows);  // ComparePage::setDiffRows 호출

    emit uiCompareClicked(effectiveLeftPath, rightPath_);
}

QList<DiffRow> ComparePage::parseDiffResult(const Json::Value &result, const QString &fileType)
{
    QList<DiffRow> rows;
    try {
        const Json::Value &changes = result["changes"];
        if (!changes.isArray())
            return rows;

        for (const auto &change : changes) {
            DiffRow row;

            // 1) type → state 매핑 (deleted 도 지원)
            if (change.isMember("type")) {
                std::string t = change["type"].asString();
                if (t == "added")
                    row.state = "ADDED";
                else if (t == "removed" || t == "deleted")
                    row.state = "REMOVED";
                else if (t == "modified")
                    row.state = "CHANGED";
                else
                    row.state = "SAME";
            }

            // YAML / 나머지(text, python 등) 포맷이 서로 다르므로 분기
            if (fileType == "yaml") {
                // ---------------- YAML ----------------
                int baseLine    = -1;
                int compareLine = -1;

                // 새 스키마 우선: baseLineNumber / compareLineNumber
                if (change.isMember("baseLineNumber") && !change["baseLineNumber"].isNull())
                    baseLine = change["baseLineNumber"].asInt();
                if (change.isMember("compareLineNumber") && !change["compareLineNumber"].isNull())
                    compareLine = change["compareLineNumber"].asInt();

                // 없으면 예전 스키마(oldLineNumber / newLineNumber)로 폴백
                if (baseLine == -1 && compareLine == -1) {
                    if (change.isMember("oldLineNumber") && !change["oldLineNumber"].isNull())
                        baseLine = change["oldLineNumber"].asInt();
                    if (change.isMember("newLineNumber") && !change["newLineNumber"].isNull())
                        compareLine = change["newLineNumber"].asInt();
                }

                row.leftLineNumber  = (compareLine > 0 ? compareLine : -1);
                row.rightLineNumber = (baseLine > 0 ? baseLine : -1);
                row.line = (row.leftLineNumber > 0 ? row.leftLineNumber : row.rightLineNumber);

                // key(경로) 설정
                if (change.isMember("path") && !change["path"].isNull())
                    row.key = QString::fromStdString(change["path"].asString());
                else if (row.line > 0)
                    row.key = QString::number(row.line);

                // 값(oldValue/newValue 또는 baseValue/compareValue 둘 다 지원)
                const Json::Value *oldV = nullptr;
                const Json::Value *newV = nullptr;

                if (change.isMember("oldValue") || change.isMember("newValue")) {
                    if (change.isMember("oldValue"))
                        oldV = &change["oldValue"];
                    if (change.isMember("newValue"))
                        newV = &change["newValue"];
                } else {
                    if (change.isMember("baseValue"))
                        oldV = &change["baseValue"];
                    if (change.isMember("compareValue"))
                        newV = &change["compareValue"];
                }

                if (oldV && !oldV->isNull()) {
                    Json::StreamWriterBuilder builder;
                    builder["indentation"] = "";
                    row.origin = QString::fromStdString(Json::writeString(builder, *oldV));
                }
                if (newV && !newV->isNull()) {
                    Json::StreamWriterBuilder builder;
                    builder["indentation"] = "";
                    row.target = QString::fromStdString(Json::writeString(builder, *newV));
                }

            } else {
                // ---------------- TEXT / PYTHON / 기타 ----------------

                // 1) 먼저 예전 스키마(lineNumber / oldLine / newLine) 체크
                if (change.isMember("lineNumber")) {
                    row.line = change["lineNumber"].asInt();
                    if (row.line > 0)
                        row.key = QString::number(row.line);

                    if (change.isMember("oldLine"))
                        row.origin = QString::fromStdString(change["oldLine"].asString());
                    if (change.isMember("newLine"))
                        row.target = QString::fromStdString(change["newLine"].asString());

                    if (row.state == "ADDED") {
                        row.leftLineNumber = row.line;
                    } else if (row.state == "REMOVED") {
                        row.rightLineNumber = row.line;
                    } else {
                        row.leftLineNumber = row.rightLineNumber = row.line;
                    }
                } else {
                    // 2) 새 스키마(baseLineNumber / compareLineNumber, baseValue / compareValue)
                    int baseLine    = -1;
                    int compareLine = -1;

                    if (change.isMember("baseLineNumber") && !change["baseLineNumber"].isNull())
                        baseLine = change["baseLineNumber"].asInt();
                    if (change.isMember("compareLineNumber") &&
                        !change["compareLineNumber"].isNull())
                        compareLine = change["compareLineNumber"].asInt();

                    // 대표 line & key
                    row.line = (compareLine > 0 ? compareLine : baseLine);
                    if (change.isMember("path") && !change["path"].isNull())
                        row.key = QString::fromStdString(change["path"].asString());
                    else if (row.line > 0)
                        row.key = QString::number(row.line);

                    if (change.isMember("baseValue") && !change["baseValue"].isNull())
                        row.origin = QString::fromStdString(change["baseValue"].asString());
                    if (change.isMember("compareValue") && !change["compareValue"].isNull())
                        row.target = QString::fromStdString(change["compareValue"].asString());

                    // 라인 번호 매핑 (좌: compare, 우: base)
                    if (row.state == "ADDED") {
                        // 왼쪽에만 있는 줄
                        row.leftLineNumber  = compareLine;
                        row.rightLineNumber = -1;
                    } else if (row.state == "REMOVED") {
                        // 오른쪽에만 있는 줄
                        row.leftLineNumber  = -1;
                        row.rightLineNumber = baseLine;
                    } else {
                        // CHANGED or SAME
                        row.leftLineNumber  = compareLine;
                        row.rightLineNumber = baseLine;
                    }
                }
            }

            rows.append(row);
        }
    } catch (...) {
        // 필요하면 로깅 추가 가능
    }
    return rows;
}

void ComparePage::performDiff(const QString &leftPath, const QString &rightPath)
{
    showCompareEditor(true);

    QFile leftFile(leftPath);
    if (!leftFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QString leftContent = QString::fromUtf8(leftFile.readAll());

    setRightFile(rightPath);
    recalcDiff(leftContent, leftPath);
}

void ComparePage::performFolderDiff(const QString &leftPath, const QString &rightPath)
{
    showCompareEditor(false);

    QString lPath = leftPath;
    QString rPath = rightPath;

    if (services::WorkspaceService::isSupportedArchive(lPath.toStdString()))
        lPath = extractArchiveToTemp(lPath);
    if (services::WorkspaceService::isSupportedArchive(rPath.toStdString()))
        rPath = extractArchiveToTemp(rPath);

    if (lPath.isEmpty() || rPath.isEmpty())
        return;

    lastLeftFolderPath_  = lPath;
    lastRightFolderPath_ = rPath;

    auto result = services::DiffService::diffDirectories(rPath.toStdString(), lPath.toStdString());

    if (!result.success) {
        QMessageBox::critical(this, "Error", QString::fromStdString(result.errorMessage));
        return;
    }

    if (diffPanel_) {
        diffPanel_->setupFolderDiffMode(result.data, lPath, rPath);
    }
}

QString ComparePage::extractArchiveToTemp(const QString &archivePath)
{
    QString tempBase = QDir::tempPath() + "/RoboEditor_Compare/";
    QDir().mkpath(tempBase);
    QString uniqueId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    services::WorkspaceMetadata metadata;
    metadata.id     = uniqueId.toStdString();
    metadata.name   = QFileInfo(archivePath).baseName().toStdString();
    metadata.target = "";

    services::ServiceResult result = services::WorkspaceService::importWorkspace(
            archivePath.toStdString(), tempBase.toStdString(), metadata);

    if (!result.success)
        return QString();

    QString extractedPath = tempBase + uniqueId;
    tempFolders_.append(extractedPath);
    return extractedPath;
}

void ComparePage::cleanupTempFolders()
{
    for (const QString &folder : tempFolders_) {
        if (QDir(folder).exists())
            QDir(folder).removeRecursively();
    }
    tempFolders_.clear();
}

ComparePage::ControllerPathInfo ComparePage::extractControllerInfo(const QString &path) const
{
    ControllerPathInfo info;
    if (path.isEmpty())
        return info;
    QString     normalized = QDir::fromNativeSeparators(QDir::cleanPath(path));
    QStringList parts      = normalized.split('/', Qt::SkipEmptyParts);

    for (int i = 0; i < parts.size(); ++i) {
        if (parts[i].compare("backup", Qt::CaseInsensitive) == 0) {
            if (i + 1 < parts.size())
                info.serial = parts[i + 1];
            break;
        }
    }
    return info;
}

QPair<QString, QString> ComparePage::determineColumnHeaders(const QString &leftPath,
                                                            const QString &rightPath) const
{
    ControllerPathInfo leftInfo  = extractControllerInfo(leftPath);
    ControllerPathInfo rightInfo = extractControllerInfo(rightPath);

    QString leftHeader =
            leftInfo.serial.isEmpty() ? QFileInfo(leftPath).fileName() : leftInfo.serial;
    QString rightHeader =
            rightInfo.serial.isEmpty() ? QFileInfo(rightPath).fileName() : rightInfo.serial;

    return {leftHeader, rightHeader};
}

void ComparePage::onDiffRowClicked(const DiffRow &row)
{
    // 어떤 줄로 스크롤할지 결정
    int line = -1;

    // DiffRow에 이런 필드가 있다면 우선 사용 (좌측/우측 라인)
    if (row.leftLineNumber > 0) {
        line = row.leftLineNumber;
    } else if (row.rightLineNumber > 0) {
        line = row.rightLineNumber;
    } else if (row.line > 0) {
        // 최소한 line 필드라도 있으면 사용
        line = row.line;
    }

    if (line <= 0)
        return;

    // 좌측/우측 에디터 모두 같은 라인으로 스크롤
    if (leftText_) {
        leftText_->scrollToLine(line);
    }
    if (rightText_) {
        rightText_->scrollToLine(line);
    }
}
void ComparePage::showCompareEditor(bool show)
{
    QWidget *tabPanel = rightSplit_->widget(0);

    if (show) {
        tabPanel->show();
        rightSplit_->setSizes({1, 1});  // equal split
    } else {
        tabPanel->hide();
        rightSplit_->setSizes({0, 1});  // right only
    }
}

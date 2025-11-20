#include "ModifyPage.h"

#include <QAbstractButton>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>

#include "CodeEditor.h"
#include "ComparePage.h"
#include "LogManager.h"
#include "mainwindow.h"

ModifyPage::ModifyPage(QWidget *parent) : QWidget(parent), currentDoc(nullptr)
{
    setAcceptDrops(true);
    buildUi();

    // 탭 전환 시 현재 문서 갱신 및 ComparePage 업데이트
    connect(tabWidget, &QTabWidget::currentChanged, [=](int index) {
        if (index < 0 || index >= documents.size()) {
            currentDoc           = nullptr;
            editor_              = nullptr;
            currentDocumentIndex = -1;
            return;
        }

        currentDocumentIndex = index;
        currentDoc           = documents[index];
        editor_              = qobject_cast<QPlainTextEdit *>(tabWidget->widget(index));

        QWidget *tabPage = tabWidget->widget(index);
        editor_          = tabPage ? tabPage->findChild<CodeEditor *>() : nullptr;

        updateTitle();

        // ComparePage가 열려있으면 현재 편집기 업데이트
        // comparePane_가 유효하고 삭제되지 않았는지 확인
        ComparePage *pane = comparePane_;
        if (pane && pane == comparePane_) {
            CodeEditor *currentEditor = tabPage ? tabPage->findChild<CodeEditor *>() : nullptr;
            if (currentEditor) {
                // ComparePage가 여전히 유효한지 재확인 후 호출
                if (currentEditor && comparePane_ == pane) {
                    pane->setLeftEditor(currentEditor);
                    pane->recalcDiff(currentLeftText(), currentLeftPath());
                }
            }
        }
    });

    connect(tabWidget, &QTabWidget::tabCloseRequested, this, [=](int index) { closeFile(index); });
}

void ModifyPage::setCurrentDocument(int index)
{
    if (index >= 0 && index < documents.size())
        currentDocumentIndex = index;
}
Document::Document(const QString &filePath, QObject *parent) :
    QObject(parent),
    filePath(filePath),
    content(""),
    isModified(false),
    cursorColumn(0),
    cursorRow(0)
{
}

Document *ModifyPage::openDocument(const QString &path)
{
    if (path.isEmpty()) {
        return nullptr;
    }

    // 기존의 있는 파일인지 확인
    for (int i = 0; i < documents.size(); i++) {
        if (documents[i]->gfilePath() == path) {
            tabWidget->setCurrentIndex(i);
            return documents[i];
        }
    }

    Document *doc = new Document(path);
    if (!doc->load()) {
        delete doc;
        return nullptr;
    }

    // 컨테이너 위젯 생성 (QLabel + CodeEditor)
    QWidget     *tabPage    = new QWidget(tabWidget);
    QVBoxLayout *pageLayout = new QVBoxLayout(tabPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    // 파일 경로 레이블 생성
    QLabel *pathLabel = new QLabel(tabPage);
    pathLabel->setObjectName("pathLabel");  // 나중에 찾기 위한 이름 설정
    if (MainWindow::dark) {
        //다크
        pathLabel->setStyleSheet("QLabel {"
                                 "  padding: 4px 8px;"
                                 "  background-color: #2d2d2d;"
                                 "  border-bottom: 1px solid #3a3a3a;"
                                 "  font-size: 9pt;"
                                 "  color: #a0a0a0;"
                                 "}");
    } else {
        //라이트
        pathLabel->setStyleSheet("QLabel {"
                                 "  padding: 4px 8px;"
                                 "  background-color: #f0f0f0;"
                                 "  border-bottom: 1px solid #d0d0d0;"
                                 "  font-size: 9pt;"
                                 "  color: #666;"
                                 "}");
    }
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    // 경로 포맷팅
    QString displayPath = formatPath(doc->gfilePath());
    pathLabel->setText(displayPath);
    pageLayout->addWidget(pathLabel);

    // CodeEditor 생성
    auto *neweditor_ = new CodeEditor(tabPage);
    neweditor_->setLoadedText(doc->gcontent(), path);
    pageLayout->addWidget(neweditor_);

    // 컨테이너를 탭에 추가
    int index = tabWidget->addTab(tabPage, doc->gfileName());
    tabWidget->setCurrentIndex(index);

    connect(neweditor_, &QPlainTextEdit::textChanged, [this, doc, neweditor_]() {
        doc->setContent(neweditor_->toPlainText());
        doc->setModified(true);
        updateTitle();

        if (comparePane_) {
            emit editorTextChangedForDiff();
        }
    });

    connect(neweditor_, &DropTextEdit::fileDropped, this, [this](const QString &path) {
        openDocument(path);
    });

    currentDoc = doc;
    editor_    = neweditor_;
    updateTitle();
    documents.append(doc);
    setCurrentDocument(documents.size() - 1);

    if (comparePane_) {
        ComparePage *pane = comparePane_;
        pane->setLeftEditor(qobject_cast<CodeEditor *>(neweditor_));
        pane->recalcDiff(currentLeftText(), currentLeftPath());
    }

    return doc;
}

QString ModifyPage::currentLeftText() const
{
    // Document를 신뢰해서 문자열 스냅샷만 전달
    if (currentDoc)
        return currentDoc->gcontent();
    // (혹은 탭 위젯에서 직접 읽어도 됨)
    auto *ed = qobject_cast<QPlainTextEdit *>(tabWidget->currentWidget());
    return ed ? ed->toPlainText() : QString();
}

QString ModifyPage::currentLeftPath() const
{
    if (currentDoc)
        return currentDoc->gfilePath();
    return QString();
}

void ModifyPage::buildUi()
{
    auto outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    mainSplit_  = new QSplitter(Qt::Horizontal, this);
    editorHost_ = new QWidget(mainSplit_);
    auto ev     = new QVBoxLayout(editorHost_);
    ev->setContentsMargins(0, 0, 0, 0);

    // 탭 위젯
    tabWidget = new QTabWidget(editorHost_);
    tabWidget->setTabsClosable(true);
    ev->addWidget(tabWidget);

    mainSplit_->addWidget(editorHost_);
    outer->addWidget(mainSplit_);
}

void ModifyPage::showCompare()
{
    // 케이스 1: 이미 ComparePage가 열려있으면 -> 즉시 재비교
    ComparePage *pane = comparePane_;
    if (pane && pane == comparePane_) {
        pane->recalcDiff(currentLeftText(), currentLeftPath());
        return;
    }

    // 케이스 2: ComparePage가 닫혀있거나 없음
    QString targetPath;

    // 이전에 비교한 파일이 있으면 선택창 표시
    if (!lastComparedPath_.isEmpty()) {
        // 간단한 질문 다이얼로그
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("파일 비교"));
        msgBox.setText(
                tr("이전에 비교한 파일이 있습니다:\n%1\n\n이전 파일을 다시 사용하시겠습니까?")
                        .arg(QFileInfo(lastComparedPath_).fileName()));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        msgBox.button(QMessageBox::Yes)->setText(tr("이전 파일 사용"));
        msgBox.button(QMessageBox::No)->setText(tr("새 파일 선택"));
        msgBox.button(QMessageBox::Cancel)->setText(tr("취소"));

        int result = msgBox.exec();

        if (result == QMessageBox::Yes) {
            // 이전 파일 재사용
            targetPath = lastComparedPath_;
        } else if (result == QMessageBox::No) {
            // 새 파일 선택 - C:/backup에서 시작
            targetPath =
                    QFileDialog::getOpenFileName(this, tr("Select file to compare"), "C:/backup");
        } else {
            // 취소
            return;
        }
    } else {
        // 이전 파일이 없으면 바로 파일 선택 - C:/backup에서 시작
        targetPath = QFileDialog::getOpenFileName(this, tr("Select file to compare"), "C:/backup");
    }

    if (targetPath.isEmpty())
        return;

    // ComparePage 생성 및 비교 실행
    lastComparedPath_ = targetPath;
    ensureCompare(targetPath);
    comparePane_->recalcDiff(currentLeftText(), currentLeftPath());
}

void ModifyPage::ensureCompare(const QString &targetPath)
{
    // 현재 활성 탭의 편집기 가져오기
    QWidget    *tabPage    = tabWidget->currentWidget();
    CodeEditor *leftEditor = tabPage ? tabPage->findChild<CodeEditor *>() : nullptr;

    if (comparePane_) {
        // 이미 존재하면 경로만 업데이트하고 좌측 편집기도 갱신
        comparePane_->setTargetPath(targetPath);
        comparePane_->setLeftEditor(leftEditor);
        comparePane_->show();
        return;
    }

    // 새로 생성
    comparePane_ = new ComparePage(this);
    comparePane_->setTargetPath(targetPath);
    comparePane_->setLeftEditor(leftEditor);  // 좌측 편집기 설정
    mainSplit_->addWidget(comparePane_);
    mainSplit_->setStretchFactor(0, 1);
    mainSplit_->setStretchFactor(1, 1);

    // ComparePage 시그널 연결
    connect(comparePane_, &ComparePage::closed, this, &ModifyPage::closeCompare);
    connect(comparePane_, &ComparePage::targetPathChanged, this, [this](const QString &path) {
        lastComparedPath_ = path;
    });

    // 폴더 비교 트리에서 더블클릭 → ModifyPage가 파일을 열도록 연결
    connect(comparePane_, &ComparePage::requestOpenFile, this, &ModifyPage::openFromTree);

    // 탭 변경 시 ComparePage 업데이트는 생성자에서 이미 연결되어 있음

    // Debouncing 타이머 초기화 (한 번만)
    if (!diffDebounceTimer_) {
        diffDebounceTimer_ = new QTimer(this);
        diffDebounceTimer_->setSingleShot(true);  // 한 번만 실행
        diffDebounceTimer_->setInterval(500);     // 500ms 대기

        // 타이머 timeout 시 실제 diff 실행
        connect(diffDebounceTimer_, &QTimer::timeout, this, [this] {
            ComparePage *pane = comparePane_;
            if (pane && pane == comparePane_) {
                pane->recalcDiff(currentLeftText(), currentLeftPath());
            }
        });

        // 텍스트 변경 시 타이머 재시작 (Debouncing) - 한 번만 연결
        // 연결을 저장하여 나중에 해제할 수 있도록 함
        editorTextChangedConnection_ =
                connect(this, &ModifyPage::editorTextChangedForDiff, this, [this] {
                    ComparePage *pane = comparePane_;
                    if (pane && pane == comparePane_ && diffDebounceTimer_) {
                        diffDebounceTimer_->stop();   // 기존 타이머 중지
                        diffDebounceTimer_->start();  // 새로 시작 (500ms 후 실행)
                    }
                });
    }

    // 저장된 설정 복원은 showCompare 호출 후에 별도로 처리
    // 여기서는 복원하지 않고, showCompare에서 처리하도록 함
}

void ModifyPage::closeCompare()
{
    if (!comparePane_)
        return;

    // ComparePage 포인터를 로컬에 저장
    ComparePage *paneToDelete = comparePane_;

    // 먼저 포인터를 nullptr로 설정하여 다른 곳에서 접근하는 것을 방지
    comparePane_ = nullptr;

    // Debounce 타이머 정리
    if (diffDebounceTimer_) {
        diffDebounceTimer_->stop();  // 실행 중인 타이머 중지
    }

    // 하이라이트 제거 (비교 파일 닫을 때 ModifyPage의 하이라이트 제거)
    // paneToDelete가 유효한지 확인 후 호출
    if (paneToDelete) {
        paneToDelete->clearHighlights();

        // ComparePage 시그널 연결 해제 (크래시 방지)
        disconnect(paneToDelete, &ComparePage::closed, this, &ModifyPage::closeCompare);
        disconnect(paneToDelete, &ComparePage::targetPathChanged, this, nullptr);

        // 경로는 보존 (lastComparedPath_에 이미 저장되어 있음)
        paneToDelete->hide();
        paneToDelete->setParent(nullptr);  // 스플리터에서 분리
        paneToDelete->deleteLater();
    }

    // editorTextChangedForDiff 연결은 해제하지 않음 (다음 ComparePage 생성 시 재사용)
    // 하지만 comparePane_가 nullptr이므로 람다에서 자동으로 무시됨
}

void ModifyPage::showCompareFolders()
{
    // 현재 열린 파일이 있는지 확인
    bool hasOpenFile = (currentDoc != nullptr && !currentDoc->gfilePath().isEmpty());

    QString leftPath;
    QString rightPath;

    if (hasOpenFile) {
        // 파일이 열려있는 경우: 해당 파일의 폴더 vs 다른 폴더/압축 파일
        QFileInfo currentFileInfo(currentDoc->gfilePath());
        leftPath = currentFileInfo.absolutePath();  // 현재 파일의 폴더

        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("폴더 비교"));
        msgBox.setText(tr("현재 파일의 폴더와 비교할 폴더/압축 파일을 선택하세요:\n현재: %1")
                               .arg(leftPath));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

        if (msgBox.exec() != QMessageBox::Ok) {
            return;
        }

        // 비교할 폴더/압축 파일 선택
        QFileDialog dialog(this, tr("Select folder or archive to compare"), "C:/backup");
        dialog.setFileMode(QFileDialog::Directory);
        dialog.setOption(QFileDialog::ShowDirsOnly, false);
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setNameFilter(tr("Folders and Archives (*.zip *.tar *.tar.gz *.tgz)"));

        if (dialog.exec() == QDialog::Accepted) {
            QStringList paths = dialog.selectedFiles();
            if (!paths.isEmpty()) {
                rightPath = paths.first();
            }
        }

        if (rightPath.isEmpty()) {
            return;
        }

    } else {
        // 파일이 열려있지 않은 경우: 두 폴더/압축 파일 선택
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("폴더 비교"));
        msgBox.setText(tr("열린 파일이 없습니다.\n비교할 두 폴더/압축 파일을 선택해주세요."));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

        if (msgBox.exec() != QMessageBox::Ok) {
            return;
        }

        // 첫 번째 폴더/압축 파일 선택
        QFileDialog dialog1(this, tr("Select first folder or archive (compare)"), "C:/backup");
        dialog1.setFileMode(QFileDialog::Directory);
        dialog1.setOption(QFileDialog::ShowDirsOnly, false);
        dialog1.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog1.setNameFilter(tr("Folders and Archives (*.zip *.tar *.tar.gz *.tgz)"));

        if (dialog1.exec() == QDialog::Accepted) {
            QStringList paths = dialog1.selectedFiles();
            if (!paths.isEmpty()) {
                leftPath = paths.first();
            }
        }

        if (leftPath.isEmpty()) {
            return;
        }

        // 두 번째 폴더/압축 파일 선택
        QFileDialog dialog2(this, tr("Select second folder or archive (base)"), "C:/backup");
        dialog2.setFileMode(QFileDialog::Directory);
        dialog2.setOption(QFileDialog::ShowDirsOnly, false);
        dialog2.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog2.setNameFilter(tr("Folders and Archives (*.zip *.tar *.tar.gz *.tgz)"));

        if (dialog2.exec() == QDialog::Accepted) {
            QStringList paths = dialog2.selectedFiles();
            if (!paths.isEmpty()) {
                rightPath = paths.first();
            }
        }

        if (rightPath.isEmpty()) {
            return;
        }
    }

    // ComparePage가 없으면 생성
    if (!comparePane_) {
        comparePane_ = new ComparePage(this);
        comparePane_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mainSplit_->addWidget(comparePane_);
        mainSplit_->setStretchFactor(0, 1);
        mainSplit_->setStretchFactor(1, 0);

        connect(comparePane_, &ComparePage::closed, this, &ModifyPage::closeCompare);
    }

    // 폴더 비교 수행
    comparePane_->show();
    comparePane_->performFolderDiff(leftPath, rightPath);
}

void ModifyPage::openFromTree(const QString &path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream   in(&f);
        const QString text = in.readAll();
        f.close();

        QWidget *tabPage = tabWidget->currentWidget();
        auto    *ed      = tabPage ? tabPage->findChild<CodeEditor *>() : nullptr;

        if (ed) {
            ed->setLoadedText(text, path);
            if (currentDoc) {
                currentDoc->setContent(text);
                currentDoc->setFilePath(path);
                currentDoc->setModified(false);
                updateTitle();
            }
        }
    }

    if (comparePane_) {
        ComparePage *pane = comparePane_;
        if (pane == comparePane_) {
            QWidget    *tabPage       = tabWidget->currentWidget();
            CodeEditor *currentEditor = tabPage ? tabPage->findChild<CodeEditor *>() : nullptr;
            pane->setLeftEditor(currentEditor);
            pane->recalcDiff(currentLeftText(), currentLeftPath());
        }
    }
}

void ModifyPage::compareWithFromTree(const QString &path)
{
    lastComparedPath_ = path;
    ensureCompare(path);
    comparePane_->recalcDiff(currentLeftText(), currentLeftPath());
}

int ModifyPage::lineNumberAreaWidth() const
{
    auto *ed = editor_ ? editor_ : qobject_cast<QPlainTextEdit *>(tabWidget->currentWidget());
    if (!ed)
        return 12;
    int digits = 1, max = qMax(1, ed->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 6 + 3 + ed->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void ModifyPage::closeDocument(int index)
{
    documents.removeAt(index);
}
void ModifyPage::closeDocument(Document *doc)
{
    int idx = documents.indexOf(doc);
    if (idx != -1) {
        documents.removeAt(idx);
    }
}

void ModifyPage::closeFile()
{
    if (documents.isEmpty() || currentDocumentIndex < 0 || currentDocumentIndex >= documents.size())
        return;

    int index = currentDocumentIndex;

    closeFile(index);
}
void ModifyPage::closeAll()
{
    for (auto d : documents) {
        closeFile();
    }
}

void ModifyPage::closeFile(int index)
{
    if (documents.isEmpty())
        return;

    currentDocumentIndex = index;
    Document *doc        = documents[currentDocumentIndex];

    if (doc->gisModified()) {
        auto result = QMessageBox::question(
                this,
                "저장 확인",
                doc->gfileName() + " 파일이 수정되었습니다.\n저장하시겠습니까?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (result == QMessageBox::Yes)
            saveFile();
        else if (result == QMessageBox::Cancel)
            return;
    }

    tabWidget->removeTab(index);
    closeDocument(index);
    delete doc;

    if (!documents.isEmpty()) {
        currentDoc = documents.last();
        editor_    = qobject_cast<QPlainTextEdit *>(tabWidget->widget(tabWidget->count() - 1));
    } else {
        currentDoc = nullptr;
        editor_    = nullptr;
    }
    updateTitle();
}
static QString shortenBackupPath(const QString &fullPath)
{
    QString base = "C:/backup/";

    if (fullPath.startsWith(base, Qt::CaseInsensitive)) {
        return fullPath.mid(base.length());
    }
    return fullPath;  // 백업 경로가 아니면 원문 그대로
}
Document *ModifyPage::currentDocument()
{
    if (currentDocumentIndex >= 0 && currentDocumentIndex < documents.size()) {
        return documents[currentDocumentIndex];
    }
    return nullptr;
}
void ModifyPage::openFile()
{
    QString fileName =
            QFileDialog::getOpenFileName(nullptr,           // 부모 위젯 포인터
                                         "Select File",     // 대화 상자 제목
                                         QDir::homePath(),  //초기 디렉토리 경로

                                         //표시할 파일 형식 필터
                                         //"설명 (확장자1 확장자2);;다른설명 (확장자3)"

                                         "All Files (*);;YAML Files (*.yaml *.yml);;Python Files "
                                         "(*.py *.srl);;ZIP Archives (*.zip)");

    Document *doc = openDocument(fileName);
    if (!doc)
        return;
}

void ModifyPage::createNewFile()
{
    Document *doc = new Document("");
    doc->setContent("");
    doc->setModified(true);

    // 2) 탭 페이지 생성
    QWidget     *tabPage = new QWidget(tabWidget);
    QVBoxLayout *layout  = new QVBoxLayout(tabPage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 3) 상단 경로 라벨
    QLabel *pathLabel = new QLabel("untitled", tabPage);
    pathLabel->setObjectName("pathLabel");

    // 4) CodeEditor 생성
    CodeEditor *editor = new CodeEditor(tabPage);
    editor->setPlainText("");
    layout->addWidget(editor);

    // 5) 탭 추가
    int index = tabWidget->addTab(tabPage, "untitled");
    tabWidget->setCurrentIndex(index);

    // 6) signal 연결
    connect(editor, &QPlainTextEdit::textChanged, [this, doc, editor]() {
        doc->setContent(editor->toPlainText());
        doc->setModified(true);
        updateTitle();
        if (comparePane_) {
            emit editorTextChangedForDiff();
        }
    });

    connect(editor, &DropTextEdit::fileDropped, this, [this](const QString &path) {
        openDocument(path);
    });

    // 7) 내부 관리 변수 업데이트
    currentDoc = doc;
    editor_    = editor;
    documents.append(doc);
    setCurrentDocument(documents.size() - 1);

    updateTitle();  // 탭 제목 갱신

    LogManager::append("New document created: untitled");
}

void ModifyPage::saveFile()
{
    if (documents.isEmpty() || currentDocumentIndex < 0 ||
        currentDocumentIndex >= documents.size()) {
        return;
    }

    Document *doc = documents[currentDocumentIndex];
    if (!doc)
        return;

    QString filePath = doc->gfilePath();
    if (filePath.isEmpty()) {
        saveAsFile();
        return;
    }

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Failed to open file:" << filePath;
        return;
    }

    QTextStream out(&file);
    out << doc->gcontent();  // 문서 내용 쓰기

    QString shortPath = shortenBackupPath(filePath);
    QString msg       = QString("[%1] is saved").arg(shortPath);
    LogManager::append(msg);
    doc->setModified(false);
    updateTitle();
    file.close();

    if (comparePane_) {
        ComparePage *pane = comparePane_;
        if (pane == comparePane_) {
            pane->recalcDiff(currentLeftText(), currentLeftPath());
        }
    }
}
void ModifyPage::saveAsFile()
{
    saveAsFile(currentDocumentIndex);
}
void ModifyPage::saveAsFile(int index)
{
    if (index < 0 || index >= documents.size())
        return;

    Document *doc = documents[index];
    if (!doc)
        return;

    QString currentPath = doc->gfilePath();
    QString initialDir;

    if (!currentPath.isEmpty()) {
        QFileInfo info(currentPath);
        initialDir = info.absolutePath();
    } else {
        initialDir = QDir::homePath();
    }

    QString newFilePath = QFileDialog::getSaveFileName(
            nullptr, "Save As", initialDir, "All Files (*);;Text Files (*.txt)");

    if (newFilePath.isEmpty()) {
        return;
    }

    QFile file(newFilePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Failed to save file:" << newFilePath;
        return;
    }

    QFileInfo infoAfter(newFilePath);
    QString   newName = infoAfter.fileName();

    QFileInfo infoBefore(currentPath);
    QString   beforeName = infoBefore.fileName();

    QTextStream out(&file);
    out << doc->gcontent();

    QString shortBefore = shortenBackupPath(currentPath);
    QString shortAfter  = shortenBackupPath(newFilePath);

    QString msg = QString("[%1] is saved to [%2]").arg(shortBefore).arg(shortAfter);
    LogManager::append(msg);

    doc->setFilePath(newFilePath);
    doc->setModified(false);
    updateTitle();
    file.close();
}
void ModifyPage::saveAll()
{
    if (documents.isEmpty())
        return;

    for (int curIdx = 0; curIdx < documents.size(); curIdx++) {
        Document *doc = documents[curIdx];
        if (!doc)
            continue;  // return이 아닌 continue

        QString filePath = doc->gfilePath();

        // untitled 파일 별도 처리
        if (filePath.isEmpty()) {
            saveAsFile(curIdx);
            continue;
        }

        // 파일 저장 시도
        QFile file(filePath);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            qWarning() << "Failed to open file:" << filePath;
            QString msg = QString("Save Failed : [%1] cant open").arg(filePath);
            LogManager::append(msg);
            continue;  // 다음 파일 계속 저장
        }

        QTextStream out(&file);
        out << doc->gcontent();
        file.close();

        doc->setModified(false);
    }

    updateTitle();
}
bool ModifyPage::hasUnsavedChanges(Document *doc)
{
    if (!doc)
        return false;
    return doc->gisModified();
}

void ModifyPage::closeCurrentTab()
{
    int index = tabWidget->currentIndex();

    if (index < 0)
        return;

    if (documents.isEmpty() || index >= documents.size())
        return;

    if (index >= 0)
        closeFile(index);
}

void ModifyPage::onTextChanged()
{
    if (currentDoc) {
        currentDoc->setContent(editor_->toPlainText());
        currentDoc->setModified(true);
    }
}
void ModifyPage::updateTitle()
{
    for (int i = 0; i < documents.size(); ++i) {
        Document *doc  = documents[i];
        QString   name = doc->gfileName();
        if (doc->gisModified())
            name += " *";
        tabWidget->setTabText(i, name);

        QWidget *tabPage = tabWidget->widget(i);
        if (tabPage) {
            QLabel *pathLabel = tabPage->findChild<QLabel *>("pathLabel");
            if (pathLabel) {
                QString displayPath = formatPath(doc->gfilePath());
                pathLabel->setText(displayPath);
            }
        }
    }
}
void findContent() {}
bool Document::load()
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    content = file.readAll();
    file.close();
    isModified = false;

    return true;
}

//아래는 Document 멤버면수의 getter, setter 함수입니다.
QString Document::gfilePath() const
{
    return filePath;
}

QString Document::gfileName() const
{
    return QFileInfo(filePath).fileName();
}

QString Document::gcontent() const
{
    return content;
}

bool Document::gisModified() const
{
    return isModified;
}

int Document::gcursorRow() const
{
    return cursorRow;
}

int Document::gcursorColumn() const
{
    return cursorColumn;
}
void Document::setFilePath(const QString &path)
{
    filePath = path;
}
void Document::setModified(bool modified)
{
    isModified = modified;
}
void Document::setContent(const QString &newContent)
{
    content    = newContent;
    isModified = true;
}
void ModifyPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();  // ✅ 파일이면 허용
}

void ModifyPage::dropEvent(QDropEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (mime->hasUrls()) {
        QList<QUrl> urls     = mime->urls();
        QString     filePath = urls.first().toLocalFile();
        qDebug() << "Dropped file:" << filePath;

        // 실제 처리 (파일 열기 등)
        Document *doc = openDocument(filePath);
        if (!doc)
            return;
    }
    event->acceptProposedAction();
}
QString ModifyPage::formatPath(const QString &fullPath) const
{
    if (fullPath.isEmpty()) {
        return "No file opened";
    }

    QString displayPath = fullPath;

    // "C:/backup" 제거
    if (displayPath.startsWith("C:/backup", Qt::CaseInsensitive)) {
        displayPath.remove(0, 9);
    } else if (displayPath.startsWith("C:\\backup", Qt::CaseInsensitive)) {
        displayPath.remove(0, 9);
    }

    // 맨 앞 / 제거
    if (displayPath.startsWith('/') || displayPath.startsWith('\\')) {
        displayPath.remove(0, 1);
    }

    // / 또는 \를 " > "로 변경
    displayPath.replace('/', " > ");
    displayPath.replace('\\', " > ");

    return displayPath;
}

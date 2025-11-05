#include "ModifyPage.h"

#include <QAbstractButton>
#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>

#include "CodeEditor.h"
#include "ComparePage.h"

ModifyPage::ModifyPage(QWidget *parent) : QWidget(parent), currentDoc(nullptr)
{
    setAcceptDrops(true);
    buildUi();

    // 탭 전환 시 현재 문서 갱신
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

        updateTitle();
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

    auto *neweditor_ = new CodeEditor();
    neweditor_->setPlainText(doc->gcontent());

    int index = tabWidget->addTab(neweditor_, doc->gfileName());
    tabWidget->setCurrentIndex(index);

    connect(neweditor_, &QPlainTextEdit::textChanged, [=]() {
        doc->setContent(neweditor_->toPlainText());
        doc->setModified(true);
        updateTitle();

        emit editorTextChangedForDiff();
    });

    connect(neweditor_, &DropTextEdit::fileDroppedToLeft, this, [this](const QString &p) {
        emit editorFileDropped(p);
    });
    connect(neweditor_, &DropTextEdit::fileDroppedToRight, this, [this](const QString &p) {
        emit editorFileDropped(p);
    });

    currentDoc = doc;
    editor_    = neweditor_;
    updateTitle();
    documents.append(doc);
    setCurrentDocument(documents.size() - 1);

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

void ModifyPage::buildUi()
{
    auto outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    mainSplit_  = new QSplitter(Qt::Horizontal, this);
    editorHost_ = new QWidget(mainSplit_);
    auto ev     = new QVBoxLayout(editorHost_);
    ev->setContentsMargins(0, 0, 0, 0);

    // 여기서 탭을 **한 번만** 만든다
    tabWidget = new QTabWidget(editorHost_);
    tabWidget->setTabsClosable(true);
    ev->addWidget(tabWidget);

    mainSplit_->addWidget(editorHost_);  // [0]은 항상 ModifyPage(탭)
    outer->addWidget(mainSplit_);
}

void ModifyPage::showCompare()
{
    // 케이스 1: 이미 ComparePage가 열려있으면 -> 즉시 재비교
    if (comparePane_ && comparePane_->isVisible()) {
        comparePane_->recalcDiff(currentLeftText());
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
            targetPath = QFileDialog::getOpenFileName(this, tr("Select file to compare"), "C:/backup");
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
    comparePane_->recalcDiff(currentLeftText());
}

void ModifyPage::ensureCompare(const QString &targetPath)
{
    // 현재 활성 탭의 편집기 가져오기
    CodeEditor *leftEditor = qobject_cast<CodeEditor *>(tabWidget->currentWidget());
    
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
    mainSplit_->setStretchFactor(1, 0);

    connect(comparePane_, &ComparePage::closed, this, &ModifyPage::closeCompare);
    connect(comparePane_, &ComparePage::targetPathChanged, this, [this](const QString &path) {
        lastComparedPath_ = path;
    });
    
    // 탭이 변경될 때마다 좌측 편집기 갱신
    connect(tabWidget, &QTabWidget::currentChanged, this, [this]() {
        if (comparePane_ && comparePane_->isVisible()) {
            CodeEditor *currentEditor = qobject_cast<CodeEditor *>(tabWidget->currentWidget());
            comparePane_->setLeftEditor(currentEditor);
        }
    });
    
    // Debouncing 타이머 초기화
    if (!diffDebounceTimer_) {
        diffDebounceTimer_ = new QTimer(this);
        diffDebounceTimer_->setSingleShot(true);  // 한 번만 실행
        diffDebounceTimer_->setInterval(500);     // 500ms 대기
        
        // 타이머 timeout 시 실제 diff 실행
        connect(diffDebounceTimer_, &QTimer::timeout, this, [this] {
            if (comparePane_ && comparePane_->isVisible()) {
                comparePane_->recalcDiff(currentLeftText());
            }
        });
    }
    
    // 텍스트 변경 시 타이머 재시작 (Debouncing)
    connect(this, &ModifyPage::editorTextChangedForDiff, this, [this] {
        if (comparePane_ && comparePane_->isVisible() && diffDebounceTimer_) {
            diffDebounceTimer_->stop();   // 기존 타이머 중지
            diffDebounceTimer_->start();  // 새로 시작 (500ms 후 실행)
        }
    });
}

void ModifyPage::closeCompare()
{
    if (!comparePane_)
        return;

    // Debounce 타이머 정리
    if (diffDebounceTimer_) {
        diffDebounceTimer_->stop();  // 실행 중인 타이머 중지
    }

    // 경로는 보존 (lastComparedPath_에 이미 저장되어 있음)
    comparePane_->hide();
    comparePane_->setParent(nullptr);  // 스플리터에서 분리
    comparePane_->deleteLater();
    comparePane_ = nullptr;
}

void ModifyPage::openFromTree(const QString &path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream   in(&f);
        const QString text = in.readAll();
        f.close();

        auto *ed = qobject_cast<QPlainTextEdit *>(tabWidget->currentWidget());
        if (ed) {
            ed->setPlainText(text);
            if (currentDoc) {
                currentDoc->setContent(text);
                currentDoc->setModified(true);
                updateTitle();
            }
        }
    }
}

void ModifyPage::compareWithFromTree(const QString &path)
{
    lastComparedPath_ = path;
    ensureCompare(path);
    comparePane_->recalcDiff(currentLeftText());
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

void ModifyPage::saveFile()
{
    Document *doc = documents[currentDocumentIndex];
    if (!doc)
        return;

    QString filePath = doc->gfilePath();
    if (filePath.isEmpty()) {
        qWarning() << "File path is empty";
        return;
    }

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Failed to open file:" << filePath;
        return;
    }

    QTextStream out(&file);
    out << doc->gcontent();  // 문서 내용 쓰기

    doc->setModified(false);
    updateTitle();
    file.close();
}
void ModifyPage::saveAsFile()
{
    Document *doc = documents[currentDocumentIndex];
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
            nullptr, "Save As", initialDir, "Text Files (*.txt);;All Files (*)");

    if (newFilePath.isEmpty()) {
        return;
    }

    QFile file(newFilePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Failed to save file:" << newFilePath;
        return;
    }

    QTextStream out(&file);
    out << doc->gcontent();
    doc->setFilePath(newFilePath);
    doc->setModified(false);
    updateTitle();
    file.close();

    qDebug() << "File saved as:" << newFilePath;
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
        this->tabWidget->setTabText(i, name);
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

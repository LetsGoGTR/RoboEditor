#include "ModifyPage.h"

#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QShortcut>

ModifyPage::ModifyPage(QWidget *parent) : QWidget(parent), currentDoc(nullptr)
{
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(tabWidget);
    setLayout(layout);

    auto openShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_O), this);
    connect(openShortcut, &QShortcut::activated, this, &ModifyPage::openFile);

    auto saveShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
    connect(saveShortcut, &QShortcut::activated, this, &ModifyPage::saveFile);

    auto saveAsShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), this);
    connect(saveAsShortcut, &QShortcut::activated, this, &ModifyPage::saveAsFile);

    auto closeShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
    connect(closeShortcut, &QShortcut::activated, this, [this]() {
        int index = tabWidget->currentIndex();
        if (index >= 0)
            closeFile(index);
    });

    auto quitShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
    connect(quitShortcut, &QShortcut::activated, this, []() { QApplication::quit(); });

    // 탭 전환 시 현재 문서 갱신
    connect(tabWidget, &QTabWidget::currentChanged, [=](int index) {
        if (index < 0 || index >= documents.size()) {
            currentDoc           = nullptr;
            editor               = nullptr;
            currentDocumentIndex = -1;
            return;
        }

        currentDocumentIndex = index;
        currentDoc           = documents[index];
        editor               = qobject_cast<QPlainTextEdit *>(tabWidget->widget(index));

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

    QPlainTextEdit *newEditor = new QPlainTextEdit();
    newEditor->setPlainText(doc->gcontent());

    int index = tabWidget->addTab(newEditor, doc->gfileName());
    tabWidget->setCurrentIndex(index);

    connect(newEditor, &QPlainTextEdit::textChanged, [=]() {
        doc->setContent(newEditor->toPlainText());
        doc->setModified(true);
        updateTitle();
    });

    currentDoc = doc;
    editor     = newEditor;
    updateTitle();
    documents.append(doc);
    setCurrentDocument(documents.size() - 1);

    return doc;
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
        editor     = qobject_cast<QPlainTextEdit *>(tabWidget->widget(tabWidget->count() - 1));
    } else {
        currentDoc = nullptr;
        editor     = nullptr;
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
void ModifyPage::onTextChanged()
{
    if (currentDoc) {
        currentDoc->setContent(editor->toPlainText());
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

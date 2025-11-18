#ifndef MODIFYPAGE_H
#define MODIFYPAGE_H
#pragma once
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>

//Document : 개별 파일 관리
//FileManager : 여러 Document 관리
//ModifyPage : UI

class ComparePage;
class LineNumberArea;
class ModifyPage;

class Document : public QObject
{
    Q_OBJECT

      public:
        explicit Document(const QString &filePath, QObject *parent = nullptr);

        //getter
        QString gfilePath() const;
        QString gfileName() const;
        QString gcontent() const;
        bool    gisModified() const;
        int     gcursorRow() const;
        int     gcursorColumn() const;

        //setter
        void setContent(const QString &content);
        void setFilePath(const QString &dir);
        void setCursorPosition(int r, int c);
        void setModified(bool modified);

        bool load();
        bool save();
        bool saveAs(const QString &newPath);

      private:
        QString filePath;      // 이 파일의 경로
        QString content;       // 이 파일의 내용
        bool    isModified;    // 이 파일이 수정되었는지
        int     cursorColumn;  // 이 파일의 커서 위치
        int     cursorRow;
};

class EditorManager
{
  public:
    // 문서 관리
    void closeDocument(int index);
    void closeDocument(Document *doc);

    // 현재 문서
    Document *currentDocument();
    void      setCurrentDocument(int index);

    // 접근
    QList<Document *> getAllDocuments();
    int               getDocumentCount();
};

class ModifyPage : public QWidget
{
    Q_OBJECT

      private:
        QPlainTextEdit   *editor;
        Document         *currentDoc;
        QList<Document *> documents;
        int               currentDocumentIndex = -1;
        void              setCurrentDocument(int index);
        void              closeDocument(int index);
        void              closeDocument(Document * doc);
        QString           formatPath(const QString &fullPath) const;
        void              buildUi();
        void              ensureCompare(const QString &targetPath);
        QString           currentLeftText() const;
        QString           currentLeftPath() const;
        QSplitter        *mainSplit_{nullptr};  // [0] editorHost, [1] comparePane(옵션)
        QWidget          *editorHost_{nullptr};
        QPlainTextEdit   *editor_{nullptr};  // 현재 탭의 에디터(CodeEditor)
        ComparePage      *comparePane_{nullptr};
        QString           lastComparedPath_;  // 마지막으로 비교한 파일 경로 저장

        // Debouncing을 위한 타이머
        QTimer *diffDebounceTimer_{nullptr};
        QMetaObject::Connection editorTextChangedConnection_;  // editorTextChangedForDiff 연결 추적

        // 라인번호
        LineNumberArea *lineArea_{nullptr};
        int             lineNumberAreaWidth() const;
        void            lineNumberAreaPaintEvent(QPaintEvent *);

      signals:
        void uiModifyClicked(const QString &target);
        void editorFileDropped(const QString &path);  // DropTextEdit 드롭 중계
        void editorTextChangedForDiff();              // (선택) 편집 변경→디프 갱신

      public:
        void setEditorManager(EditorManager * manager);
        explicit ModifyPage(QWidget *parent = nullptr);
        QTabWidget *tabWidget;

        // 외부에서 접근 가능한 메서드들
        Document *openDocument(const QString &path);
        Document *currentDocument();  // private에서 public으로 이동
        void      closeFile(int index);
        void      openFile();
        void      saveFile();
        void      saveAsFile();
        bool      hasUnsavedChanges(Document * doc);
        void      updateTitle();
        void      onTabChanged(int index);
        // 트리뷰 연동용 (함수만 준비)
        void openFromTree(const QString &path);
        void compareWithFromTree(const QString &path);

      private:
        int cursorLine;
        int cursorColumn;

      public slots:
        void closeCurrentTab();

      private slots:
        void onTextChanged();

      public slots:
        void showCompare();  // Compare 버튼 진입: "닫힘→파일선택", "열림→재비교"
        void showCompareFolders();  // 폴더 비교 버튼: 파일 유무에 따라 동작 분기
        void closeCompare();        // ComparePane의 [X] 클릭 시 호출

      protected:
        void dragEnterEvent(QDragEnterEvent * event) override;
        void dropEvent(QDropEvent * event) override;
};

#endif  // MODIFYPAGE_H

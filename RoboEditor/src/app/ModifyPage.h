#ifndef MODIFYPAGE_H
#define MODIFYPAGE_H

#include <QTabWidget>
#include <QTextStream>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QPlainTextEdit>
#include <QWidget>

//Document : 개별 파일 관리
//FileManager : 여러 Document 관리
//ModifyPage : UI

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

      signals:
        void uiModifyClicked(const QString &target);

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

      private:
        int cursorLine;
        int cursorColumn;

      public slots:
        void closeCurrentTab();

      private slots:
        void onTextChanged();
};

#endif  // MODIFYPAGE_H

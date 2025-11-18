#ifndef CODEEDITOR_H
#define CODEEDITOR_H
#pragma once
#include <QPlainTextEdit>
#include <QWidget>

#include "DiffHighlighter.h"
#include "DropTextEdit.h"

class CodeEditor;

class LineNumberArea : public QWidget
{
  public:
    explicit LineNumberArea(CodeEditor *editor);
    QSize sizeHint() const override;

  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    CodeEditor *editor_;
};

class CodeEditor : public DropTextEdit
{
    Q_OBJECT
      public:
        explicit CodeEditor(QWidget *parent = nullptr);
        int  lineNumberAreaWidth() const;
        void lineNumberAreaPaintEvent(QPaintEvent * event);

        // Diff 하이라이트 기능 (DiffHighlighter 사용)
        void                   setDiffHighlighter(core::DiffHighlighter * highlighter);
        core::DiffHighlighter *getDiffHighlighter() const;
        void                   clearDiffHighlights();
        void                   scrollToLine(int lineNumber);
        void                   applyTheme(bool isDark);

      protected:
        void resizeEvent(QResizeEvent * event) override;
        void wheelEvent(QWheelEvent * event) override;
      private slots:
        void updateLineNumberAreaWidth(int);
        void updateLineNumberArea(const QRect &rect, int dy);
        void highlightCurrentLine();
        void updateAllHighlights();

      private:
        LineNumberArea        *lineNumberArea_{nullptr};
        core::DiffHighlighter *diffHighlighter_{nullptr};  // DiffHighlighter 객체
        QFont                  font_;
        bool                   isDarkMode_ = false;
};

#endif  // CODEEDITOR_H

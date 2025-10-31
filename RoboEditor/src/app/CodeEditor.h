#ifndef CODEEDITOR_H
#define CODEEDITOR_H
#pragma once
#include "DropTextEdit.h"
#include <QWidget>
#include <QPlainTextEdit>

class CodeEditor;

class LineNumberArea : public QWidget {
  public:
    explicit LineNumberArea(CodeEditor* editor);
    QSize sizeHint() const override;
  protected:
    void paintEvent(QPaintEvent* event) override;
  private:
    CodeEditor* editor_;
};

class CodeEditor : public DropTextEdit {
    Q_OBJECT
  public:
    explicit CodeEditor(QWidget* parent = nullptr);
    int  lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent* event);
  protected:
    void resizeEvent(QResizeEvent* event) override;
  private slots:
    void updateLineNumberAreaWidth(int);
    void updateLineNumberArea(const QRect& rect, int dy);
    void highlightCurrentLine();
  private:
    LineNumberArea* lineNumberArea_{nullptr};
};


#endif  // CODEEDITOR_H

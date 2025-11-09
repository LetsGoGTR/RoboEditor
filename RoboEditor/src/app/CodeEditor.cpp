#include "CodeEditor.h"

#include <QTextBlock>

#include <QDebug>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>

LineNumberArea::LineNumberArea(CodeEditor *e) : QWidget(e), editor_(e) {}
QSize LineNumberArea::sizeHint() const
{
    return QSize(editor_->lineNumberAreaWidth(), 0);
}
void LineNumberArea::paintEvent(QPaintEvent *ev)
{
    editor_->lineNumberAreaPaintEvent(ev);
}

CodeEditor::CodeEditor(QWidget *parent) :
    DropTextEdit(parent),
    lineNumberArea_(new LineNumberArea(this))
{
    // CodeEditor는 편집 가능해야 함 (DropTextEdit는 기본 readOnly)
    setReadOnly(false);

    font_.setPointSize(10);
    setFont(font_);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1, max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 6 + 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea_->scroll(0, dy);
    else
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *ev)
{
    DropTextEdit::resizeEvent(ev);
    const QRect cr = contentsRect();
    lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *ev)
{
    QPainter p(lineNumberArea_);
    // 팔레트 브러시는 QBrush라 darker가 없으니 color()로 꺼내세요
    p.fillRect(ev->rect(), palette().base().color().darker(105));
    QTextBlock blk    = firstVisibleBlock();
    int        bn     = blk.blockNumber();
    qreal      top    = blockBoundingGeometry(blk).translated(contentOffset()).top();
    qreal      bottom = top + blockBoundingRect(blk).height();
    while (blk.isValid() && top <= ev->rect().bottom()) {
        if (blk.isVisible() && bottom >= ev->rect().top()) {
            p.setPen(palette().mid().color());
            p.drawText(0,
                       int(top),
                       lineNumberArea_->width() - 4,
                       fontMetrics().height(),
                       Qt::AlignRight,
                       QString::number(bn + 1));
        }
        blk    = blk.next();
        top    = bottom;
        bottom = top + blockBoundingRect(blk).height();
        ++bn;
    }
}

void CodeEditor::highlightCurrentLine()
{
    updateAllHighlights();
}

void CodeEditor::updateAllHighlights()
{
    QList<QTextEdit::ExtraSelection> extra;

    // 1. Diff 하이라이트 추가 (DiffHighlighter 사용)
    if (diffHighlighter_) {
        QMap<int, QString> lineStates = diffHighlighter_->getLineStates();

        if (!lineStates.isEmpty()) {
            QTextBlock block      = document()->firstBlock();
            int        lineNumber = 1;

            while (block.isValid()) {
                if (lineStates.contains(lineNumber)) {
                    QString state = lineStates[lineNumber];
                    QColor  color = diffHighlighter_->getColorForState(state);

                    if (color.isValid()) {
                        QTextEdit::ExtraSelection sel;
                        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
                        sel.format.setBackground(color);

                        sel.cursor = QTextCursor(block);
                        sel.cursor.clearSelection();
                        extra.append(sel);
                    }
                }

                block = block.next();
                lineNumber++;
            }
        }
    }

    // 2. 현재 커서 라인 하이라이트 (읽기 전용이 아닐 때만)
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(palette().alternateBase());
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        extra.append(sel);
    }

    setExtraSelections(extra);
}

void CodeEditor::setDiffHighlighter(core::DiffHighlighter *highlighter)
{
    // 기존 연결 해제
    if (diffHighlighter_) {
        disconnect(diffHighlighter_,
                   &core::DiffHighlighter::highlightChanged,
                   this,
                   &CodeEditor::updateAllHighlights);
    }

    diffHighlighter_ = highlighter;

    // 새 연결 설정
    if (diffHighlighter_) {
        connect(diffHighlighter_,
                &core::DiffHighlighter::highlightChanged,
                this,
                &CodeEditor::updateAllHighlights);
    }

    updateAllHighlights();
}

core::DiffHighlighter *CodeEditor::getDiffHighlighter() const
{
    return diffHighlighter_;
}

void CodeEditor::clearDiffHighlights()
{
    if (diffHighlighter_) {
        diffHighlighter_->clearLineStates();
    } else {
        updateAllHighlights();
    }
}

void CodeEditor::scrollToLine(int lineNumber)
{
    if (lineNumber < 1)
        return;

    QTextBlock block = document()->findBlockByLineNumber(lineNumber - 1);
    if (block.isValid()) {
        QTextCursor cursor(block);
        setTextCursor(cursor);
        centerCursor();
    }
}
void CodeEditor::wheelEvent(QWheelEvent *ev)
{
    if (ev->modifiers() & Qt::ControlModifier) {
        int delta    = ev->angleDelta().y();
        int numSteps = delta / 120;

        if (numSteps != 0) {  // 실제 변경이 있을 때만

            int newSize = qBound(6, font_.pointSize() + numSteps, 40);

            if (font_.pointSize() != newSize) {  // 크기가 실제로 바뀔 때만

                font_.setPointSize(newSize);

                setFont(font_);
                lineNumberArea_->setFont(font_);
                updateLineNumberAreaWidth(0);
            }
        }

        ev->accept();
    } else {
        QPlainTextEdit::wheelEvent(ev);
    }
}

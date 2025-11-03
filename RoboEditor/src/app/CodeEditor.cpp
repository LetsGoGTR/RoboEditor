#include "CodeEditor.h"

#include <QTextBlock>

#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>

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
    if (isReadOnly())
        return;
    QList<QTextEdit::ExtraSelection> extra;
    QTextEdit::ExtraSelection        sel;
    sel.format.setBackground(palette().alternateBase());
    sel.format.setProperty(QTextFormat::FullWidthSelection, true);
    sel.cursor = textCursor();
    sel.cursor.clearSelection();
    extra.append(sel);
    setExtraSelections(extra);
}

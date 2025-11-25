#include "LogTextEdit.h"

#include <QApplication>
#include <QClipboard>
#include <QMenu>

LogTextEdit::LogTextEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    font_.setPointSize(10);
    setFont(font_);

    setReadOnly(true);
    setMaximumBlockCount(1000);
    setFocusPolicy(Qt::StrongFocus);
    setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
}

void LogTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    menu->exec(event->globalPos());
    delete menu;
}

void LogTextEdit::wheelEvent(QWheelEvent *ev)
{
    if (ev->modifiers() & Qt::ControlModifier) {
        int delta    = ev->angleDelta().y();
        int numSteps = delta / 120;

        if (numSteps != 0) {
            int newSize = qBound(6, font_.pointSize() + numSteps, 40);

            if (font_.pointSize() != newSize) {
                font_.setPointSize(newSize);
                setFont(font_);
            }
        }
        ev->accept();
    } else {
        QPlainTextEdit::wheelEvent(ev);
    }
}

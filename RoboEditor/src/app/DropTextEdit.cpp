#include "DropTextEdit.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QTextOption>

DropTextEdit::DropTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setReadOnly(true);
    setAcceptDrops(true);
    setWordWrapMode(QTextOption::NoWrap);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    setPlaceholderText("Drag a file here or double click in directory list");
#endif
}

void DropTextEdit::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    } else {
        e->ignore();
    }
}

void DropTextEdit::dropEvent(QDropEvent* e) {
    if (!e->mimeData()->hasUrls()) {
        e->ignore();
        return;
    }

    const QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty()) {
        e->ignore();
        return;
    }

    const QString path = urls.first().toLocalFile();
    if (path.isEmpty()) {
        e->ignore();
        return;
    }

    // 양쪽 다 true일 수 있음 → 네가 원하는 "아무 트리에서 아무 쪽으로 드롭" 시나리오
    if (acceptAsLeft_) {
        emit fileDroppedToLeft(path);
    }
    if (acceptAsRight_) {
        emit fileDroppedToRight(path);
    }

    e->acceptProposedAction();
}

void DropTextEdit::setLoadedText(const QString& text, const QString& srcPath) {
    setPlainText(text);
    lastPath_ = srcPath;
}

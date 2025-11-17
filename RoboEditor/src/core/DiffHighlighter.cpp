#include "DiffHighlighter.h"

#include "mainwindow.h"

namespace core
{

    DiffHighlighter::HighlightColors::HighlightColors(bool isDark)
    {
        if (isDark) {
            // 다크 모드 색상
            added   = QColor(34, 84, 61);  // 어두운 초록색
            removed = QColor(88, 28, 36);  // 어두운 빨강색
            changed = QColor(82, 82, 40);  // 어두운 노란색
            error   = QColor(88, 59, 40);  // 어두운 주황색
        } else {
            // 라이트 모드 색상
            added   = QColor(220, 252, 231);  // 연한 초록색
            removed = QColor(254, 226, 226);  // 연한 빨강색
            changed = QColor(255, 250, 205);  // 연한 노란색
            error   = QColor(255, 237, 213);  // 연한 주황색
        }
    }

    DiffHighlighter::DiffHighlighter(QObject *parent) : QObject(parent), colors_(MainWindow::dark)
    {
    }

    DiffHighlighter::~DiffHighlighter() {}

    void DiffHighlighter::setLineStates(const QMap<int, QString> &lineStates)
    {
        lineStates_ = lineStates;
        emit highlightChanged();
    }

    void DiffHighlighter::clearLineStates()
    {
        lineStates_.clear();
        emit highlightChanged();
    }

    QMap<int, QString> DiffHighlighter::getLineStates() const
    {
        return lineStates_;
    }

    QColor DiffHighlighter::getColorForState(const QString &state) const
    {
        if (state == "ADDED") {
            return colors_.added;
        } else if (state == "REMOVED") {
            return colors_.removed;
        } else if (state == "CHANGED") {
            return colors_.changed;
        } else if (state == "ERROR") {
            return colors_.error;
        }

        return QColor();  // 투명색
    }

    bool DiffHighlighter::hasLineState(int lineNumber) const
    {
        return lineStates_.contains(lineNumber);
    }

    QString DiffHighlighter::getLineState(int lineNumber) const
    {
        return lineStates_.value(lineNumber, QString());
    }

    void DiffHighlighter::setColors(const HighlightColors &colors)
    {
        colors_ = colors;
        emit highlightChanged();
    }
    void DiffHighlighter::updateColorsForTheme(bool isDark)
    {
        if (isDark) {
            colors_ = HighlightColors(true);  // 다크 모드 색상
        } else {
            colors_ = HighlightColors(false);  // 라이트 모드 색상
        }
        emit highlightChanged();
    }
    DiffHighlighter::HighlightColors DiffHighlighter::getColors() const
    {
        return colors_;
    }

}  // namespace core

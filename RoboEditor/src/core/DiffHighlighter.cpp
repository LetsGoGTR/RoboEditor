#include "DiffHighlighter.h"

namespace core
{

DiffHighlighter::DiffHighlighter(QObject *parent) : QObject(parent) {}

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

DiffHighlighter::HighlightColors DiffHighlighter::getColors() const
{
    return colors_;
}

}


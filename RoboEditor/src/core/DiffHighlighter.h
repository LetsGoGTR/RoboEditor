#ifndef DIFFHIGHLIGHTER_H
#define DIFFHIGHLIGHTER_H
#pragma once
#include <QTextEdit>

#include <QColor>
#include <QMap>
#include <QObject>
#include <QString>

namespace core
{

    // Diff 하이라이트 관리 클래스
    class DiffHighlighter : public QObject
    {
        Q_OBJECT

          public:
            explicit DiffHighlighter(QObject *parent = nullptr);
            ~DiffHighlighter();

            // 상태별 색상 정의

            struct HighlightColors
            {
                QColor added;    // 추가된 라인
                QColor removed;  // 삭제된 라인
                QColor changed;  // 변경된 라인
                QColor error;    // 에러 라인

                HighlightColors(bool isDark = false);
            };

            // 라인 상태 설정
            void setLineStates(const QMap<int, QString> &lineStates);
            void clearLineStates();

            // 라인 상태 가져오기
            QMap<int, QString> getLineStates() const;

            // 특정 라인의 색상 가져오기
            QColor getColorForState(const QString &state) const;

            // 특정 라인의 상태 확인
            bool    hasLineState(int lineNumber) const;
            QString getLineState(int lineNumber) const;

            // 색상 커스터마이징
            void            setColors(const HighlightColors &colors);
            HighlightColors getColors() const;

            void updateColorsForTheme(bool isDark);

          signals:
            void highlightChanged();  // 하이라이트 정보가 변경되었을 때

          private:
            QMap<int, QString> lineStates_;  // 라인번호 -> "ADDED"/"REMOVED"/"CHANGED"
            HighlightColors    colors_;
    };

}  // namespace core

#endif  // DIFFHIGHLIGHTER_H

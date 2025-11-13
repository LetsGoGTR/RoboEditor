#ifndef DROPTEXTEDIT_H
#define DROPTEXTEDIT_H
#pragma once
#include <QPlainTextEdit>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

class DropTextEdit : public QPlainTextEdit
{
    Q_OBJECT
      public:
        explicit DropTextEdit(QWidget *parent = nullptr);

        QString lastLoadedPath() const
        {
            return lastPath_;
        }

        // 드랍 이벤트를 받을지 말지
        void setAccept(bool v)
        {
            accept_ = v;
        }

        // 파일 로드 후 텍스트 채울 때 호출
        void setLoadedText(const QString &text, const QString &srcPath);

      signals:
        void fileDropped(const QString &path);

      protected:
        void dragEnterEvent(QDragEnterEvent * e) override;
        void dragMoveEvent(QDragMoveEvent * e) override;
        void dropEvent(QDropEvent * e) override;

      private:
        bool    accept_ = true;
        QString lastPath_;
};

#endif  // DROPTEXTEDIT_H

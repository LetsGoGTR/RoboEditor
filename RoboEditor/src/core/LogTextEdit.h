#ifndef LOGTEXTEDIT_H
#define LOGTEXTEDIT_H

#include <QContextMenuEvent>
#include <QFont>
#include <QPlainTextEdit>
#include <QWheelEvent>

class LogTextEdit : public QPlainTextEdit
{
    Q_OBJECT
      public:
        explicit LogTextEdit(QWidget *parent = nullptr);

      protected:
        void contextMenuEvent(QContextMenuEvent * event) override;
        void wheelEvent(QWheelEvent * ev) override;

      private:
        QFont font_;
};

#endif

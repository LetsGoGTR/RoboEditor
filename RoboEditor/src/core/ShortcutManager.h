#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QAction>
#include <QObject>

class ShortcutManager : public QObject
{
    Q_OBJECT
      public:
        explicit ShortcutManager(QObject *parent = nullptr);
        void registerTo(QWidget * widget);

      signals:
        void openRequested();    //수정     : backup폴더부터 시작, modifypage_와 연동
        void saveRequested();    //예외처리  : 안열려있으면  Qmessage
        void saveAsRequested();  //예외처리  : 안열려있으면  Qmessage
        void closeRequested();   //modifypage_와 연동
        void quitRequested();

      private:
        void setupShortcuts();

        QAction *openAction;
        QAction *saveAction;
        QAction *saveAsAction;
        QAction *closeAction;
        QAction *quitAction;
};

#endif  // SHORTCUTMANAGER_H

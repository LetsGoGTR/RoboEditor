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
        void openRequested();
        void saveRequested();
        void saveAsRequested();
        void closeRequested();
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

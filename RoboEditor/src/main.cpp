#include <QTranslator>

#include <QApplication>
#include <QLocale>
#include <QStyleHints>

#include "CenterStack.h"
#include "PasswordManager.h"
#include "app/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator       translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "RoboEditor_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    QString logoPath = CenterStack::getIconPath() + "/logo.png";
    a.setWindowIcon(QIcon(logoPath));

    MainWindow w;
    a.installEventFilter(&w);

    w.setWindowFlags(Qt::Window);
    w.showMaximized();
    return a.exec();
}

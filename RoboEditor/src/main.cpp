#include <QTranslator>

#include <QApplication>
#include <QLocale>

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
    MainWindow w;
    w.setWindowFlags(Qt::Window);  // ✅ 타이틀바, 닫기/최대화 버튼 유지
    w.showMaximized();
    return a.exec();
}

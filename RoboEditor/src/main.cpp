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

    QString stylePath = QApplication::applicationDirPath() + "/../../src/styles/style.qss";
    QFile   styleFile(stylePath);
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        a.setStyleSheet(style);
        qDebug() << "✅ Loaded QSS from:" << stylePath;
    } else {
        qWarning() << "⚠️ Failed to load QSS:" << stylePath;
    }

    MainWindow w;
    w.setWindowFlags(Qt::Window);
    w.showMaximized();
    return a.exec();
}

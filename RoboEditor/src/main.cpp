#include <QTranslator>

#include <QApplication>
#include <QLocale>
#include <QStyleHints>

#include "PasswordManager.h"
#include "app/mainwindow.h"

static void loadTheme()
{
    bool dark = qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;

    // Build correct QSS file path
    QString fileName = dark ? "dark.qss" : "light.qss";

    QString stylePath = QCoreApplication::applicationDirPath() + "/styles/" + fileName;
    QFile styleFile(stylePath);

    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        qApp->setStyleSheet(style);
    } else {
        qWarning() << "⚠️ Failed to load QSS:" << stylePath;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    loadTheme();

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
    a.installEventFilter(&w);

    w.setWindowFlags(Qt::Window);
    w.showMaximized();
    return a.exec();
}


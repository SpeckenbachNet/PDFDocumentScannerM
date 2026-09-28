#include "mainwin.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QStringLiteral>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationVersion(QLatin1String(PROJECT_VERSION));
    a.setWindowIcon(QIcon(":/icons/app_icon"));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    bool loaded = false;


    if (!translator.load(":/i18n/PDFDocumentScannerM_de.qm")) {
        qDebug() << "Übersetzung konnte nicht geladen werden!";
    } else {
        qDebug() << "Übersetzung geladen.";
    }

    qApp->installTranslator(&translator);



    for (const QString &locale : uiLanguages) {
        QString langCode = QLocale(locale).name().left(2);
        const QString baseName = "PDFDocumentScannerM_" + langCode;
        //qDebug() << baseName;
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            loaded = true;
            break;
        }

        if (!loaded) {
            if (translator.load(":/i18n/PDFDocumentScannerM_en")) {
                a.installTranslator(&translator);
            }
        }
    }
    MainWin w;
    w.setWindowTitle(
        QStringLiteral(PROJECT_NAME) +
        " " +
        QStringLiteral(PROJECT_VERSION)
    );
    w.show();
    return a.exec();
}

#include "mainwin.h"

#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#include <QStringLiteral>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(QStringLiteral(PROJECT_NAME));
    a.setApplicationVersion(QLatin1String(PROJECT_VERSION));
    a.setOrganizationName(QStringLiteral(COMPANY_NAME));
    // Settings: ~/.config/SpeckenbachNet/PDFDocumentScannerM.ini (SettingsManager)
    QSettings::setDefaultFormat(QSettings::IniFormat);
    a.setWindowIcon(QIcon(":/icons/app_icon"));
    // Linux: Wayland app id, ties the window to net.speckenbach.PDFDocumentScannerM.desktop (icon in the task bar)
    a.setDesktopFileName(QStringLiteral("net.speckenbach.PDFDocumentScannerM"));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    bool loaded = false;

    // Use the first system language we have a translation for
    for (const QString &locale : uiLanguages) {
        QString langCode = QLocale(locale).name().left(2);
        const QString baseName = "PDFDocumentScannerM_" + langCode;
        if (translator.load(":/i18n/" + baseName)) {
            loaded = true;
            break;
        }
    }

    // Fallback: English
    if (!loaded) {
        loaded = translator.load(":/i18n/PDFDocumentScannerM_en");
    }

    if (loaded) {
        a.installTranslator(&translator);
    } else {
        qDebug() << "Übersetzung konnte nicht geladen werden!";
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

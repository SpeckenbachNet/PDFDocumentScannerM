#include "mainwin.h"
#include "colorschemewatcher.h"
#include "uicolors.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QStringLiteral>
#include <QStyleHints>
#include <QTranslator>

// Follow the light / dark mode of the desktop (niri, KDE, GNOME).
// The value comes from the XDG desktop portal, so it works regardless of
// QT_QPA_PLATFORMTHEME (gtk3, kde, xdgdesktopportal) and inside the AppImage.
// The palette changes with it, so Fusion's palette is set again afterwards.
static void applyColorScheme(ColorSchemeWatcher::Scheme scheme)
{
    Qt::ColorScheme qtScheme = Qt::ColorScheme::Unknown;
    if (scheme == ColorSchemeWatcher::Dark)
        qtScheme = Qt::ColorScheme::Dark;
    else if (scheme == ColorSchemeWatcher::Light)
        qtScheme = Qt::ColorScheme::Light;

    QGuiApplication::styleHints()->setColorScheme(qtScheme);
    UiColors::applyStandardPalette();   // same colors after every switch, readable selection
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(QStringLiteral(PROJECT_NAME));
    a.setApplicationVersion(QLatin1String(PROJECT_VERSION));
    a.setOrganizationName(QStringLiteral(COMPANY_NAME));

    // Fusion has a built-in light and dark palette and follows setColorScheme()
    a.setStyle(QStringLiteral("Fusion"));

    // Settings: ~/.config/SpeckenbachNet/PDFDocumentScannerM.ini (SettingsManager)
    QSettings::setDefaultFormat(QSettings::IniFormat);

    a.setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon")));

    // Linux: Wayland app id, ties the window to net.speckenbach.PDFDocumentScannerM.desktop
    // (icon in the task bar)
    a.setDesktopFileName(QStringLiteral("net.speckenbach.PDFDocumentScannerM"));

    // Light / dark mode: now and whenever it changes
    auto *colorScheme = new ColorSchemeWatcher(&a);
    applyColorScheme(colorScheme->scheme());
    QObject::connect(colorScheme, &ColorSchemeWatcher::schemeChanged, &applyColorScheme);

    // Translation: first system language with a translation, otherwise English
    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("PDFDocumentScannerM"), QStringLiteral("_"),
                        QStringLiteral(":/i18n"))
        || translator.load(QStringLiteral(":/i18n/PDFDocumentScannerM_en"))) {
        a.installTranslator(&translator);
    } else {
        qWarning() << "Translation could not be loaded";
    }
    // Qt's own texts (standard buttons, file dialog) in the same language
    QTranslator qtTranslator;
    if (!translator.isEmpty()
        && qtTranslator.load(QLocale(translator.language()), QStringLiteral("qtbase"),
                             QStringLiteral("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        a.installTranslator(&qtTranslator);
    }

    MainWin w;
    w.setWindowTitle(QStringLiteral(PROJECT_NAME) + QLatin1Char(' ') + QStringLiteral(PROJECT_VERSION));
    w.show();

    return a.exec();
}

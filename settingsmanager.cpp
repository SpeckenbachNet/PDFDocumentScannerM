#include "settingsmanager.h"
#include <QStandardPaths>

SettingsManager::SettingsManager() {
    // Organization, application name and INI format are set in main()
    m_settings = new QSettings();
    resetToDefaults();
    readSettingsFromIni();
}

void SettingsManager::resetToDefaults() {
    scannerValues.clear();

    // Reset strings/stringlists
    m_deviceId = "";
    m_resolution = 0;
    m_scanningUnit = "";
    m_pageOrientation = "Portrait";
    m_pageFormat = "Standard";
    m_colorMode = "Color";
    m_ocrLanguage = "deu";
    m_intentDocument = false;

    resolutionValues = {"75", "100", "200", "300"};
    resolutionDefault = "";
    colorModeValues = {"Gray", "Color"};
    colorModeDefault = "";
    intentValues.clear();
    intentDefault ="";
    scanningUnitValues.clear();
    scanningUnitDefault = "";
}

void SettingsManager::clearCapabilities() {
    // Remove the options of the previously used scanner
    resolutionValues.clear();
    resolutionDefault = "";
    colorModeValues.clear();
    colorModeDefault = "";
    intentValues.clear();
    intentDefault = "";
    scanningUnitValues.clear();
    scanningUnitDefault = "";
}

bool SettingsManager::isLandscape() {
    if(m_pageOrientation == "Portrait") return false;
    else return true;
}

void SettingsManager::readSettingsFromIni() {
    m_savedDeviceId = m_settings->value("Scanner/DeviceId").toString();
    m_savedDeviceName = m_settings->value("Scanner/DeviceName").toString();
    m_deviceId = m_savedDeviceId;
    m_scanningUnit = m_settings->value("Scanner/ScanningUnit", m_scanningUnit).toString();
    m_pageOrientation = m_settings->value("Scanner/PageOrientation", m_pageOrientation).toString();
    m_pageFormat = m_settings->value("Scanner/PageFormat", m_pageFormat).toString();
    // The app's own modes (PageProcessing); older versions stored the scanner's value
    m_colorMode = m_settings->value("Scanner/ColorMode", m_colorMode).toString();
    if (m_colorMode == "Lineart") m_colorMode = "BlackWhite";
    if (m_colorMode != "Gray" && m_colorMode != "BlackWhite") m_colorMode = "Color";
    m_resolution = m_settings->value("Scanner/Resolution").toInt();
    m_scanIntent = m_settings->value("Scanner/ScanIntent", m_scanIntent).toString();
    m_ocrLanguage = m_settings->value("Scanner/OcrLanguage", m_ocrLanguage).toString();
}

// Not stored: a scanner that is missing at one search stays the user's choice for the next
void SettingsManager::setDeviceId(QString devId) {
    m_deviceId = devId;
}

void SettingsManager::chooseDevice(QString devId, QString displayName) {
    m_deviceId = devId;
    m_savedDeviceId = devId;
    m_savedDeviceName = displayName;
    m_settings->setValue("Scanner/DeviceId", devId);
    m_settings->setValue("Scanner/DeviceName", displayName);
    m_settings->sync();
}

void SettingsManager::setScanningUnit(QString scanUnit) {
    m_scanningUnit = scanUnit;
    m_settings->setValue("Scanner/ScanningUnit", scanUnit);
    m_settings->sync();
}

void SettingsManager::setColorMode(QString colMode) {
    m_colorMode = colMode;
    m_settings->setValue("Scanner/ColorMode", colMode);
    m_settings->sync();
}

void SettingsManager::setResolution(int dpi) {
    if (m_resolution == dpi) return;
    m_resolution = dpi;
    m_settings->setValue("Scanner/Resolution", dpi);
    m_settings->sync();
}

void SettingsManager::setPageOrientation(QString pageOrientation) {
    m_pageOrientation = pageOrientation;
    m_settings->setValue("Scanner/PageOrientation", pageOrientation);
    m_settings->sync();
}

void SettingsManager::setPageFormat(QString pageFormat) {
    m_pageFormat = pageFormat;
    m_settings->setValue("Scanner/PageFormat", pageFormat);
    m_settings->sync();
}

void SettingsManager::setScanIntent(QString scanIntent) {
    m_scanIntent = scanIntent;
    m_settings->setValue("Scanner/ScanIntent", scanIntent);
    m_settings->sync();
}

// Tesseract language code, e.g. "deu", "eng"
void SettingsManager::setOcrLanguage(QString ocrLanguage) {
    m_ocrLanguage = ocrLanguage;
    m_settings->setValue("Scanner/OcrLanguage", ocrLanguage);
    m_settings->sync();
}

QByteArray SettingsManager::windowGeometry() const {
    return m_settings->value("Window/Geometry").toByteArray();
}

QByteArray SettingsManager::splitterState() const {
    return m_settings->value("Window/Splitter").toByteArray();
}

void SettingsManager::saveWindowLayout(const QByteArray &geometry, const QByteArray &splitter) {
    m_settings->setValue("Window/Geometry", geometry);
    m_settings->setValue("Window/Splitter", splitter);
    m_settings->sync();
}

QString SettingsManager::lastSaveFolder() const {
    return m_settings->value("Save/LastFolder",
                             QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
}

void SettingsManager::setLastSaveFolder(const QString &folder) {
    m_settings->setValue("Save/LastFolder", folder);
    m_settings->sync();
}

// --- DER DEBUG OUTPUT ---
void SettingsManager::dump() const {
    QDebug dbg = qDebug().nospace().noquote();
    dbg << "--- SettingsManager Singleton Status ---\n";
    dbg << "Device ID:    " << (m_deviceId.isEmpty() ? "None" : m_deviceId) << "\n";
    dbg << "Resolution:   " << m_resolution << " dpi\n";

    // Enums (werden als Integer ausgegeben, sofern kein Cast vorliegt)
    dbg << "Orientation:  " << m_pageOrientation << "\n";
    dbg << "Format:       " << m_pageFormat << "\n";
    dbg << "Color Mode:   " << m_colorMode << "\n";
    dbg << "Scan Unit:    " << m_scanningUnit << "\n";
    dbg << "Scan-Intent:  " << m_scanIntent << "\n";
    dbg << "OCR Language: " << m_ocrLanguage << "\n";

    dbg << "Lists:\n";
    dbg << "  Resolutions: " << resolutionValues.join(", ") << " [" << resolutionDefault << "]" << "\n";
    dbg << "  Scan-Intent: " << intentValues.join(", ") << " [" << intentDefault << "]" << "\n";
    dbg << "  Colors:      " << colorModeValues.join(", ") << " [" << colorModeDefault << "]" << "\n";
    dbg << "  Sources:     " << scanningUnitValues.join(", ") << " [" << scanningUnitDefault << "]" << "\n";

    dbg << "Scanner Values (Map):\n";
    if (scannerValues.isEmpty()) {
        dbg << "  (Empty)\n";
    } else {
        for (auto it = scannerValues.cbegin(); it != scannerValues.cend(); ++it) {
            dbg << "  - " << it.key() << ": " << it.value() << "\n";
        }
    }
    dbg << "---------------------------------------";
}

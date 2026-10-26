#pragma once
//#include "globals.h"
#include <QStringList>
#include <QSettings>

class SettingsManager
{
public:
    static SettingsManager& instance() {
        static SettingsManager _instance;
        return _instance;
    }

    // Getter
    QString deviceId() const { return m_deviceId; }             // scanner used in this session
    QString savedDeviceId() const { return m_savedDeviceId; }   // scanner chosen by the user
    QString savedDeviceName() const { return m_savedDeviceName; }
    QString scanningUnit() const { return m_scanningUnit; }
    QString colorMode() const { return m_colorMode; }
    QString scanIntent() const { return m_scanIntent; }
    QString pageOrientation() const { return m_pageOrientation; }
    QString pageFormat() const { return m_pageFormat; }
    QString ocrLanguage() const { return m_ocrLanguage; }
    int resolution() const { return m_resolution; }
    int splitterWidth() const { return m_splitterWidth; }
    bool intentDocument() const { return m_intentDocument; }

    // Setter
    void setDeviceId(QString deviceId);   // this session only, e.g. a stand-in for a missing scanner
    void chooseDevice(QString deviceId, QString displayName);   // the user's choice, stored in the INI
    void setScanningUnit(QString scaningUnit);
    void setColorMode(QString colorMode);
    void setResolution(int resolution);
    void setPageOrientation(QString pageOrientation);
    void setPageFormat(QString pageFormat);
    void setScanIntent(QString scanIntent);
    void setOcrLanguage(QString ocrLanguage);

    // Other
    QMap<QString, QString> scannerValues;
    QStringList resolutionValues;
    QString resolutionDefault;
    QStringList colorModeValues;
    QString colorModeDefault;
    QStringList intentValues;
    QString intentDefault;
    QStringList scanningUnitValues;
    QString scanningUnitDefault;
    // QStringList scanIntentValues;
    // QString scanIntentDefault;
    void clearCapabilities();
    bool isLandscape();
    void dump() const;

private:
    SettingsManager();
    QSettings* m_settings;
    void resetToDefaults();
    void readSettingsFromIni();

    QString m_deviceId;
    QString m_savedDeviceId;
    QString m_savedDeviceName;
    QString m_scanningUnit;
    QString m_colorMode;
    QString m_scanIntent;
    QString m_pageOrientation;
    QString m_pageFormat;
    QString m_ocrLanguage = "deu";

    bool m_ocr = false;
    bool m_straightenDocument = false;
    bool m_cropDocument = false;
    bool m_intentDocument = false;
    int m_resolution = 300;
    int m_splitterWidth = 180;
};

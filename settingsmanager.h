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
    // eSCL root of an escl scanner (http://IP:port/eSCL/): from the search, the saved one, or the
    // usual /eSCL/ path
    QString esclUrl(const QString &deviceId) const;
    QString scanningUnit() const { return m_scanningUnit; }
    QString colorMode() const { return m_colorMode; }
    QString scanIntent() const { return m_scanIntent; }
    QString pageOrientation() const { return m_pageOrientation; }
    QString pageFormat() const { return m_pageFormat; }
    QString ocrLanguage() const { return m_ocrLanguage; }
    bool straighten() const { return m_straightenDocument; }   // new scans are turned straight
    int resolution() const { return m_resolution; }
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
    void setStraighten(bool straighten);

    // Window size/position and splitter (QWidget::saveGeometry / QSplitter::saveState),
    // empty before the first close
    QByteArray windowGeometry() const;
    QByteArray splitterState() const;
    void saveWindowLayout(const QByteArray &geometry, const QByteArray &splitter);

    // Save dialog: QWidget::saveGeometry / QFileDialog::saveState (view, columns, sidebar),
    // empty before the first save
    QByteArray saveDialogGeometry() const;
    QByteArray saveDialogState() const;
    void setSaveDialogLayout(const QByteArray &geometry, const QByteArray &state);

    // Folder of the last saved PDF; the user's documents folder before the first save
    QString lastSaveFolder() const;
    void setLastSaveFolder(const QString &folder);

    // Other
    QMap<QString, QString> scannerValues;
    QMap<QString, QString> scannerUrls;   // device ID -> eSCL root, as airscan-discover reports it
    QStringList resolutionValues;
    QString resolutionDefault;
    QStringList colorModeValues;
    QString colorModeDefault;
    QStringList intentValues;
    QString intentDefault;
    QStringList scanningUnitValues;
    QString scanningUnitDefault;
    // Largest scan area in mm (scanimage -x / -y), 0 = unknown. It depends on the source
    // (HP M277dw: flatbed 297 mm long, feeder 381 mm), so it is only valid for scanAreaSource
    // (the default source when the options were loaded; empty: the scanner has no sources).
    double scanAreaMaxWidth = 0;
    double scanAreaMaxHeight = 0;
    QString scanAreaSource;
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
    QString m_savedDeviceUrl;
    QString m_scanningUnit;
    QString m_colorMode;
    QString m_scanIntent;
    QString m_pageOrientation;
    QString m_pageFormat;
    QString m_ocrLanguage;   // empty: the language of the system (Ocr::fillLanguageCombo)

    bool m_ocr = false;
    bool m_straightenDocument = false;
    bool m_intentDocument = false;
    int m_resolution = 300;
};

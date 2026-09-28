// SettingsOverlay.h
#pragma once

#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QProcess>
#include <QCheckBox>
#include <QRegularExpression>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QEvent>

class MainWindow;

class SettingsOverlay : public QWidget {
    Q_OBJECT

public:
    explicit SettingsOverlay(QWidget *parent = nullptr);
    void fillComboBoxes();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void extracted(QEvent *&event);
    //void changeEvent(QEvent *event) override;

private:
    MainWindow  *m_mainWindow; // Save Pointer
    QListWidget *m_deviceList;
    QVBoxLayout *m_layout;
    QComboBox   *m_scannerCombo;
    QComboBox   *m_colorModeCombo;
    QComboBox   *m_resolutionCombo;
    QComboBox   *m_pageOrientationCombo;
    QComboBox   *m_ocrLanguageCombo;
    QComboBox   *m_paperFormatCombo;
    QComboBox   *m_scanningUnitCombo;
    QComboBox   *m_scanIntentCombo;
    QPixmap getTintedPixmap(const QIcon &icon, const QSize &size, const QColor &color);
    bool setComboBoxValue(QComboBox* box, const QVariant& value);
    void updateScannerList();
    void initializeOverlay();

    // Konstanten für das Design
    const int m_arrowWidth = 20;
    const int m_arrowHeight = 12;
    const int m_borderRadius = 12;

private slots:
    void onDeviceIdChanged(int index);
    void onResolutionChanged(int index);
    void onColorModeChanged(int index);
    void onScanningUnitChanged(int index);
    void onScanIntentChanged(int index);
    void onPageOrientationChanged(int index);
    void onPageFormatChanged(int index);

signals:
    void scannerChanged(const QString &deviceId);
};

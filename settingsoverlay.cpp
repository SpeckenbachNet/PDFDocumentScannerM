#include "settingsoverlay.h"
#include "settingsmanager.h"
#include "uicolors.h"
#include "ocr.h"
#include "updatechecker.h"
#include <QApplication>

namespace {

// Display name of a scan intent of the scanner (airscan: Document, Text and Graphic, Photo,
// Preview); the English value is what is stored and sent to scanimage. Unknown values as they are.
QString intentLabel(const QString &value) {
    if (value == "Document") return QCoreApplication::translate("SettingsOverlay", "Document");
    if (value == "Text and Graphic") return QCoreApplication::translate("SettingsOverlay", "Text and graphic");
    if (value == "Photo") return QCoreApplication::translate("SettingsOverlay", "Photo");
    if (value == "Preview") return QCoreApplication::translate("SettingsOverlay", "Preview");
    return value;
}

}

SettingsOverlay::SettingsOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    //setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setObjectName("SettingsOverlay");

    initializeOverlay();

    // Connects
    connect(m_scannerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsOverlay::onDeviceIdChanged);
    connect(m_scanningUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsOverlay::onScanningUnitChanged);
    connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsOverlay::onResolutionChanged);
    connect(m_colorModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsOverlay::onColorModeChanged);
    connect(m_scanIntentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsOverlay::onScanIntentChanged);
    connect(m_pageOrientationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsOverlay::onPageOrientationChanged);
    connect(m_paperFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsOverlay::onPageFormatChanged);
    connect(m_ocrLanguageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsOverlay::onOcrLanguageChanged);
    connect(m_straightenCheck, &QCheckBox::toggled,
            this, [](bool checked) { SettingsManager::instance().setStraighten(checked); });
    connect(m_checkUpdatesCheck, &QCheckBox::toggled, this, [this](bool checked) {
        UpdateChecker::setEnabled(checked);
        emit checkUpdatesChanged(checked);
    });
}

void SettingsOverlay::initializeOverlay()
{
    // Take m_layout and use it directly
    m_layout = new QVBoxLayout(this);

    // Space for the nose (m_arrowHeight + buffer)
    int topSpace = m_arrowHeight + 20;
    m_layout->setContentsMargins(20, topSpace, 20, 20);
    m_layout->setSpacing(5);

    // Add Widgets to m_layout
    m_layout->addWidget(new QLabel(tr("SCANNER")));
    m_scannerCombo = new QComboBox(this);
    m_scannerCombo->setObjectName("ComboBox Scanner");
    m_scannerCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_scannerCombo->setMinimumContentsLength(10);
    m_layout->addWidget(m_scannerCombo);

    m_layout->addSpacing(10);

    // Create Grid-Layout for ComboBoxes
    auto *grid = new QGridLayout;
    grid->setColumnStretch(1, 1);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(10);
    m_layout->addLayout(grid);

    // Scanning unit
    m_scanningUnitCombo = new QComboBox(this);
    grid->addWidget(new QLabel(tr("SCANNING_UNIT")), 0, 0);
    grid->addWidget(m_scanningUnitCombo, 0, 1);

    // Scanning mode/type
    m_colorModeCombo = new QComboBox(this);
    grid->addWidget(new QLabel(tr("COLORMODE")), 1, 0);
    grid->addWidget(m_colorModeCombo, 1, 1);

    // Scan Intent
    m_scanIntentCombo = new QComboBox(this);
    grid->addWidget(new QLabel(tr("SCAN_INTENT")), 2, 0);
    grid->addWidget(m_scanIntentCombo, 2, 1);

    // Resolution
    m_resolutionCombo = new QComboBox(this);
    grid->addWidget(new QLabel(tr("RESOLUTION")), 3, 0);
    grid->addWidget(m_resolutionCombo, 3, 1);

    // Orientation
    m_pageOrientationCombo = new QComboBox(this);
    grid->addWidget(new QLabel(tr("PAGE_ORIENTATION")), 4, 0);
    grid->addWidget(m_pageOrientationCombo, 4, 1);

    // Paper format
    m_paperFormatCombo = new QComboBox(this);
    grid->addWidget(new QLabel(tr("PAGE_FORMAT")), 5, 0);
    grid->addWidget(m_paperFormatCombo, 5, 1);

    // OCR language: preset of the save dialog (there the buttons decide whether OCR is used)
    m_ocrLanguageCombo = new QComboBox(this);
    grid->addWidget(new QLabel(tr("OCR_LANGUAGE")), 6, 0);
    grid->addWidget(m_ocrLanguageCombo, 6, 1);

    m_layout->addSpacing(10);

    // --- Checkboxes ---
    m_straightenCheck = new QCheckBox(tr("STRAIGHTEN_DOCUMENT"));
    m_straightenCheck->setToolTip(tr("Pages that lie slightly askew (up to 5°) are turned straight "
                                     "right after scanning."));
    m_layout->addWidget(m_straightenCheck);

    // Crop does not work yet: shown, but disabled
    auto *cropCheck = new QCheckBox(tr("CROP_DOCUMENT"));
    cropCheck->setEnabled(false);
    cropCheck->setToolTip(tr("Coming in a later version"));
    m_layout->addWidget(cropCheck);

    // Asks GitHub once a day whether there is a newer version (UpdateChecker)
    m_checkUpdatesCheck = new QCheckBox(tr("Check for updates"));
    m_checkUpdatesCheck->setToolTip(tr("Once a day the app asks GitHub whether there is a new version. "
                                       "Only your IP address is sent."));
    m_layout->addWidget(m_checkUpdatesCheck);

    m_layout->addStretch();
    applyFieldStyles();

    // CUSTOMIZE stylesheet
    // Remove background-color from the stylesheet
    // because the paintEvent draws the background (due to the nose).
    //We only use the stylesheet for the children (label, etc.)
    setStyleSheet(
        "QLabel { color: #888; font-weight: bold; font-size: 10px; }"
        "QComboBox { font-size: 12px; }"
        "QCheckBox { font-size: 12px; }"
        );
}

void SettingsOverlay::applyFieldStyles() {
    for (QComboBox *combo : findChildren<QComboBox*>()) UiColors::applyComboStyle(combo);
}

void SettingsOverlay::onDeviceIdChanged(int index) {
    QString devId = m_scannerCombo->itemData(index, Qt::UserRole).toString();
    SettingsManager::instance().chooseDevice(devId, m_scannerCombo->itemText(index));
    emit scannerChanged(devId);
    this->hide();
}

void SettingsOverlay::onResolutionChanged(int index) {
    int res = m_resolutionCombo->itemData(index, Qt::UserRole).toInt();
    SettingsManager::instance().setResolution(res);
}

void SettingsOverlay::onColorModeChanged(int index) {
    QString colMode = m_colorModeCombo->itemData(index, Qt::UserRole).toString();
    SettingsManager::instance().setColorMode(colMode);
}

void SettingsOverlay::onScanningUnitChanged(int index) {
    QString scanUnit = m_scanningUnitCombo->itemData(index, Qt::UserRole).toString();
    SettingsManager::instance().setScanningUnit(scanUnit);
}

void SettingsOverlay::onScanIntentChanged(int index) {
    QString scanIntent = m_scanIntentCombo->itemData(index, Qt::UserRole).toString();
    SettingsManager::instance().setScanIntent(scanIntent);
}

void SettingsOverlay::onPageOrientationChanged(int index) {
    QString pageOrientation = m_pageOrientationCombo->itemData(index, Qt::UserRole).toString();
    SettingsManager::instance().setPageOrientation(pageOrientation);
}

void SettingsOverlay::onPageFormatChanged(int index) {
    QString pageFormat = m_paperFormatCombo->itemData(index, Qt::UserRole).toString();
    SettingsManager::instance().setPageFormat(pageFormat);
}

void SettingsOverlay::onOcrLanguageChanged(int index) {
    QString ocrLanguage = m_ocrLanguageCombo->itemData(index, Qt::UserRole).toString();
    SettingsManager::instance().setOcrLanguage(ocrLanguage);
}

void SettingsOverlay::fillComboBoxes() {
    fillScannerCombos();
    fillGeneralCombos();
}

// Scanner and its capabilities: without a scanner these combos are emptied and disabled,
// so no options of a previously found scanner remain
void SettingsOverlay::fillScannerCombos() {
    const QList<QComboBox*> capabilityCombos = {
        m_scanningUnitCombo, m_colorModeCombo, m_scanIntentCombo, m_resolutionCombo
    };

    // Scanners
    m_scannerCombo->blockSignals(true);
    m_scannerCombo->clear();
    if (SettingsManager::instance().scannerValues.isEmpty()) {
        m_scannerCombo->addItem(tr("NO_SCANNERS_FOUND"));
        m_scannerCombo->blockSignals(false);
        for (QComboBox *box : capabilityCombos) {
            box->blockSignals(true);
            box->clear();
            box->blockSignals(false);
            box->setEnabled(false);
        }
        return;
    }
    for (QComboBox *box : capabilityCombos) box->setEnabled(true);

    for (auto [deviceId, displayName] : SettingsManager::instance().scannerValues.asKeyValueRange()) {
        //qDebug() << "DeviceID: " << deviceId << " | DisplayName: " << displayName;
        m_scannerCombo->addItem(displayName, deviceId);
        m_scannerCombo->setItemData(m_scannerCombo->count() - 1, displayName, Qt::ToolTipRole);
    }
    setComboBoxValue(m_scannerCombo, SettingsManager::instance().deviceId());
    m_scannerCombo->blockSignals(false);

    // Scan Unit
    m_scanningUnitCombo->blockSignals(true);
    m_scanningUnitCombo->clear();
    m_scanningUnitCombo->addItem(tr("AUTOMATIC"), "Automatic");
    for (const QString& scanUnit : std::as_const(SettingsManager::instance().scanningUnitValues)) {
        m_scanningUnitCombo->addItem(scanUnit, scanUnit);
    }
    setComboBoxValue(m_scanningUnitCombo, QVariant::fromValue(SettingsManager::instance().scanningUnit()));
    m_scanningUnitCombo->blockSignals(false);

    // Color Mode: the app's own modes, whatever the scanner calls them (PageProcessing)
    m_colorModeCombo->blockSignals(true);
    m_colorModeCombo->clear();
    m_colorModeCombo->addItem(tr("Color"), "Color");
    m_colorModeCombo->addItem(tr("Grayscale"), "Gray");
    m_colorModeCombo->addItem(tr("Black/white"), "BlackWhite");
    setComboBoxValue(m_colorModeCombo, QVariant::fromValue(SettingsManager::instance().colorMode()));
    m_colorModeCombo->blockSignals(false);

    // Scan Intent
    m_scanIntentCombo->blockSignals(true);
    m_scanIntentCombo->clear();
    for (const QString& scanIntent : std::as_const(SettingsManager::instance().intentValues)) {
        m_scanIntentCombo->addItem(intentLabel(scanIntent), scanIntent);   // the English value is stored
    }
    setComboBoxValue(m_scanIntentCombo, QVariant::fromValue(SettingsManager::instance().scanIntent()));
    m_scanIntentCombo->blockSignals(false);
    // Not every backend offers --scan-intent (e.g. escl in SANE 1.4)
    m_scanIntentCombo->setEnabled(m_scanIntentCombo->count() > 0);

    // Resolution
    m_resolutionCombo->blockSignals(true);
    m_resolutionCombo->clear();
    for (const QString& dpi : std::as_const(SettingsManager::instance().resolutionValues)) {
        m_resolutionCombo->addItem(dpi + "dpi", dpi);
    }
    setComboBoxValue(m_resolutionCombo, SettingsManager::instance().resolution());
    m_resolutionCombo->blockSignals(false);
}

// Settings that do not depend on the scanner: always filled
void SettingsOverlay::fillGeneralCombos() {
    // Orientation
    QColor textColor = this->palette().color(QPalette::WindowText);
    QSize iconSize(16,16);
    // Icons are black drawings: tint them in the text color (light and dark theme)
    QIcon pIcon(getTintedPixmap(QIcon(":/icons/document_portrait"), iconSize, textColor));
    QIcon lIcon(getTintedPixmap(QIcon(":/icons/document_landscape"), iconSize, textColor));
    m_pageOrientationCombo->blockSignals(true);
    m_pageOrientationCombo->clear();
    m_pageOrientationCombo->addItem(pIcon,tr("PORTRAIT"), "Portrait");
    m_pageOrientationCombo->addItem(lIcon,tr("LANDSCAPE"), "Landscape");
    setComboBoxValue(m_pageOrientationCombo, QVariant::fromValue(SettingsManager::instance().pageOrientation()));
    m_pageOrientationCombo->blockSignals(false);

    // Page Format
    m_paperFormatCombo->blockSignals(true);
    m_paperFormatCombo->clear();
    m_paperFormatCombo->addItem(tr("STANDARD"), "Standard");
    m_paperFormatCombo->addItem("A4", "A4");
    m_paperFormatCombo->addItem("A5", "A5");
    m_paperFormatCombo->addItem("A6", "A6");
    m_paperFormatCombo->addItem("Letter", "Letter");
    m_paperFormatCombo->addItem("Legal", "Legal");
    setComboBoxValue(m_paperFormatCombo, QVariant::fromValue(SettingsManager::instance().pageFormat()));
    m_paperFormatCombo->blockSignals(false);

    // OCR languages: the ones installed for tesseract
    m_ocrLanguageCombo->blockSignals(true);
    const bool hasLanguages = Ocr::fillLanguageCombo(m_ocrLanguageCombo, SettingsManager::instance().ocrLanguage());
    if (!hasLanguages) m_ocrLanguageCombo->addItem(tr("Not installed"));
    m_ocrLanguageCombo->setEnabled(hasLanguages);
    m_ocrLanguageCombo->setToolTip(hasLanguages ? QString()
        : tr("Text recognition needs the program tesseract with at least one language "
             "(e.g. the package tesseract-data-deu or tesseract-ocr-deu)."));
    m_ocrLanguageCombo->blockSignals(false);

    m_straightenCheck->blockSignals(true);
    m_straightenCheck->setChecked(SettingsManager::instance().straighten());
    m_straightenCheck->blockSignals(false);

    m_checkUpdatesCheck->blockSignals(true);
    m_checkUpdatesCheck->setChecked(UpdateChecker::isEnabled());
    m_checkUpdatesCheck->blockSignals(false);
}

bool SettingsOverlay::setComboBoxValue(QComboBox* box, const QVariant& value)
{
    if (!box) {
        qDebug() << "No box";
        return false;
    }

    int index = box->findData(value, Qt::UserRole);
    //qDebug() << "Index: " << index;
    if (index < 0) {
        qDebug() << "No UserRole found. Index is lower 0";
        return false;
    }

    box->setCurrentIndex(index);
    return true;
}

QPixmap SettingsOverlay::getTintedPixmap(const QIcon &icon, const QSize &size, const QColor &color)
{
    if (icon.isNull()) return QPixmap();

    // WICHTIG: Berücksichtige den Skalierungsfaktor des Bildschirms (z.B. 2.0 für 4K)
    qreal dpr = this->devicePixelRatioF();

    // Wir laden das Bild in der physisch benötigten Auflösung (z.B. 44x44 statt 22x22)
    QPixmap pixmap = icon.pixmap(size * dpr);

    // Wir sagen dem Pixmap, dass es sich wie ein 'size' großes Bild verhalten soll
    pixmap.setDevicePixelRatio(dpr);

    QPainter pixPainter(&pixmap);
    pixPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    pixPainter.fillRect(pixmap.rect(), color);
    pixPainter.end();

    return pixmap;
}

// ===== Events =====

void SettingsOverlay::showEvent(QShowEvent *event) {
    fillComboBoxes();
    QWidget::showEvent(event);
}

void SettingsOverlay::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Derived from Window/WindowText: QPalette::Light is nearly equal to Window under gtk3,
    // the frame vanished there. Application palette like in MainWin/ToolbarM.
    const QPalette appPal = QApplication::palette();
    QColor bgColor = appPal.color(QPalette::Window);
    bgColor.setAlpha(240);
    QColor frameColor = UiColors::mix(appPal.color(QPalette::Window),
                                      appPal.color(QPalette::WindowText),
                                      UiColors::overlayFrameShade);

    int noseX = 20; // The nose starts 20px from the left
    int borderRadius = 12;

    QPainterPath path;

    // START THE PATH AT THE NOSE - Everything before that remains transparent!
    path.moveTo(0, m_arrowHeight + borderRadius); // Starting point: Left edge below the nose

    // Arch at the top left (the “crease” under the nose)
    path.arcTo(0, m_arrowHeight, borderRadius * 2, borderRadius * 2, 180, -90);

    // Line to the nose
    path.lineTo(noseX, m_arrowHeight);
    path.lineTo(noseX + (m_arrowWidth / 2), 0); // The tip of the nose
    path.lineTo(noseX + m_arrowWidth, m_arrowHeight);

    // Continue to the upper right corner
    path.lineTo(width() - borderRadius, m_arrowHeight);
    path.arcTo(width() - borderRadius * 2, m_arrowHeight, borderRadius * 2, borderRadius * 2, 90, -90);

    // Right down, bottom long, left up
    path.lineTo(width(), height() - borderRadius);
    path.arcTo(width() - borderRadius * 2, height() - borderRadius * 2, borderRadius * 2, borderRadius * 2, 0, -90);
    path.lineTo(borderRadius, height());
    path.arcTo(0, height() - borderRadius * 2, borderRadius * 2, borderRadius * 2, 270, -90);
    path.closeSubpath();

    // Now ONLY this path is drawn. Everything outside (to the left of the nose)
    // becomes crystal clear thanks to WA_TranslucentBackground.
    painter.fillPath(path, bgColor);
    painter.setPen(QPen(frameColor, 1.2));
    painter.drawPath(path);
}

// paintEvent reads the application palette, so repaint when it changes (light/dark switch)
void SettingsOverlay::changeEvent(QEvent *event) {
    if (event->type() == QEvent::ApplicationPaletteChange) {
        applyFieldStyles();   // the style sheets hold the colors of the old mode
        update();
    }
    QWidget::changeEvent(event);
}

// void SettingsOverlay::changeEvent(QEvent *event) {
//     if (event->type() == QEvent::ActivationChange) {
//         if (!this->isActiveWindow()) {
//             this->close(); // Schließen, wenn das Fenster nicht mehr aktiv ist
//         }
//     }
//     QWidget::changeEvent(event);
// }

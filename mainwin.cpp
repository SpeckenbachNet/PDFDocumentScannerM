#include "mainwin.h"
#include "./ui_mainwin.h"
#include "settingsmanager.h"
#include "pageprocessing.h"
#include "sanedevice.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPageLayout>
#include <QPdfWriter>
#include <QPushButton>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>

namespace {

// Plain text (e.g. a scanimage message) as HTML for the status overlay. Device URLs have no
// spaces, so zero width spaces after '/' and ':' give the word wrap places to break.
QString breakableHtml(const QString &text) {
    QString html = text.toHtmlEscaped();
    html.replace('/', QStringLiteral("/​"));
    html.replace(':', QStringLiteral(":​"));
    return html;
}

// Backends name their feeders differently ("ADF", "ADF Front", "ADF Duplex",
// "Automatic Document Feeder", ...)
// PDF page of the scanned size: pixels and resolution. Without a known scan resolution the one
// stored in the image is used. Landscape pages stay landscape (QPageSize alone is portrait).
QPageLayout pageLayoutFor(const QImage &image, int dpi) {
    if (dpi <= 0) dpi = qRound(image.dotsPerMeterX() * 0.0254);
    if (dpi <= 0) dpi = 300;
    const QSizeF mm(image.width() * 25.4 / dpi, image.height() * 25.4 / dpi);
    const QPageSize size(QSizeF(qMin(mm.width(), mm.height()), qMax(mm.width(), mm.height())),
                         QPageSize::Millimeter, QString(), QPageSize::FuzzyMatch);
    const auto orientation = mm.width() > mm.height() ? QPageLayout::Landscape : QPageLayout::Portrait;
    return QPageLayout(size, orientation, QMarginsF(0, 0, 0, 0));
}

// One page per image, the image fills the page
bool writePdf(const QString &filePath, const QList<PageListWidget::Page> &pages) {
    QPdfWriter writer(filePath);
    writer.setCreator(QCoreApplication::applicationName());
    writer.setTitle(QFileInfo(filePath).completeBaseName());

    QPainter painter;
    for (int i = 0; i < pages.size(); ++i) {
        const QImage image(pages[i].filePath);
        if (image.isNull()) return false;
        writer.setPageLayout(pageLayoutFor(image, pages[i].dpi));   // applies to the next page
        if (i == 0) {
            if (!painter.begin(&writer)) return false;
        } else if (!writer.newPage()) {
            return false;
        }
        painter.drawImage(QRect(0, 0, writer.width(), writer.height()), image);
    }
    return painter.end();
}

bool isAdfSource(const QString &source) {
    return source.contains("ADF", Qt::CaseInsensitive)
        || source.contains("Feeder", Qt::CaseInsensitive)
        || source.contains("Duplex", Qt::CaseInsensitive);
}

// Many backends report a range (e.g. "50..1200 (in steps of 1)") instead of a list.
// In that case offer the common resolutions that lie within the range.
QStringList expandResolutionValues(const QStringList &values, const QString &defaultValue) {
    if (values.size() != 1) return values;

    static const QRegularExpression rangeRegex("^(\\d+)\\.\\.(\\d+)");
    QRegularExpressionMatch match = rangeRegex.match(values.first());
    if (!match.hasMatch()) return values;

    const int minRes = match.captured(1).toInt();
    const int maxRes = match.captured(2).toInt();
    QList<int> resolutions;
    for (int res : {75, 100, 150, 200, 300, 400, 600, 1200, 2400, 4800}) {
        if (res >= minRes && res <= maxRes) resolutions.append(res);
    }
    const int defaultRes = defaultValue.toInt();
    if (defaultRes > 0 && !resolutions.contains(defaultRes)) resolutions.append(defaultRes);
    std::sort(resolutions.begin(), resolutions.end());

    QStringList result;
    for (int res : std::as_const(resolutions)) result.append(QString::number(res));
    return result;
}

}

MainWin::MainWin(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWin)
{
    ui->setupUi(this);

    // Initialize (initialize.cpp)
    initializeToolBars();
    initializeForm();
    initializeConnects();

    searchForScanners();
}

QString MainWin::getPageFormat() {
    QPageSize defaultSize = QLocale().measurementSystem() == QLocale::MetricSystem
        ? QPageSize(QPageSize::A4)
        : QPageSize(QPageSize::Letter);
    return defaultSize.name();
}

void MainWin::showSettingsOverlay() {
    if (m_settingsOverlay->isVisible()) {
        m_settingsOverlay->hide();
    } else {
        // Find the global position of the upper left corner of the button.
        // Take the point (0, 0) of the button and convert it to the MainWindow.
        QPoint globalBtnPos = m_settingsBtn->mapToGlobal(QPoint(0, m_settingsBtn->height()));
        QPoint globalBtnCenter = m_settingsBtn->mapToGlobal(QPoint(m_settingsBtn->width() / 2, 0));
        int noseTipInOverlay = 30;
        int x = globalBtnCenter.x() - noseTipInOverlay;
        int y = globalBtnPos.y();

        int width = 400;
        int height = 440;

        m_settingsOverlay->setGeometry(x, y, width, height);
        m_settingsOverlay->show();
        m_settingsOverlay->raise();
    }
}

// Every page brings its own toolbar, so switching the page is all there is to do
void MainWin::showPage(QWidget *page) {
    ui->stackedWidget->setCurrentWidget(page);
}

// The large preview shows the current page (click, arrow keys, new page); none left: empty
void MainWin::showCurrentPage() {
    QListWidgetItem *page = ui->pagesListWidget->currentItem();
    if (page) {
        ui->previewGraphicsView->setImage(page->data(Qt::UserRole).toString());
    } else {
        ui->previewGraphicsView->clearImage();
    }
}

// ====== Scanner functions ======

void MainWin::searchForScanners() {
    m_scannerReady = false;   // the scanner may be gone after the search
    setBusy(true);
    m_statusOverlay->showMessage(tr("SEARCH_SCANNERS..."));

    // airscan-discover (sane-airscan) finds the network scanners in 1-3 s and reliably;
    // scanimage -L takes ~10 s and its mDNS search often misses a device
    const bool useAirscan = !QStandardPaths::findExecutable("airscan-discover").isEmpty();

    QProcess *process = new QProcess(this);

    // scanimage not installed / not executable: finished() is never emitted in this case
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        m_statusOverlay->showMessage(tr("EROOR_STARTING_SEARCH."), true, 3000);
        setBusy(false);
        process->deleteLater();
    });

    connect(process, &QProcess::finished, this, [this, process, useAirscan]() {
        process->deleteLater();
        if (process->exitStatus() != QProcess::NormalExit) {
            m_statusOverlay->showMessage(tr("EROOR_STARTING_SEARCH."), true, 3000);
            setBusy(false);
            return;
        }

        const QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
        SettingsManager::instance().scannerValues.clear();   // forget scanners of a previous search
        SettingsManager::instance().scannerUrls.clear();
        if (useAirscan) {
            saveAirscanScanners(output);
        } else {
            saveScanners(output);
        }

        if (!chooseScanner()) {
            m_statusOverlay->showMessage(output.trimmed().isEmpty() ? tr("NO_SCANNER_HARDWARE_FOUND.")
                                                                    : tr("NO_SCANNERS_DETECTED."),
                                         true, 3000);
            setBusy(false);
            return;
        }
        m_statusOverlay->showMessage(tr("LOAD_SCANNER_CAPABILITIES..."));
        QTimer::singleShot(10, this, [this]() {
            this->getScannerCapabilities(SettingsManager::instance().deviceId());
        });
    });

    if (useAirscan) {
        process->start("airscan-discover", QStringList());
    } else {
        process->start("scanimage", QStringList() << "-L");
    }
}

// airscan-discover lines: "  Brother MFC-L2710DW series = http://192.168.178.23:80/eSCL/, eSCL".
// Only eSCL entries with an IPv4 address are used, as escl device (escl:http://192.168.178.23:80):
// its ID is the URL and stays the same, the airscan IDs (airscan:e0:...) change with every search.
// escl (SANE 1.4) cannot open IPv6 URLs and does not speak WSD. The whole URL (with the eSCL path)
// is kept for scanning through airscan (SaneDevice).
void MainWin::saveAirscanScanners(const QString &output) {
    static const QRegularExpression lineRegex(
        "^\\s*(.+?)\\s*=\\s*((https?://(\\d+\\.\\d+\\.\\d+\\.\\d+)(?::\\d+)?)/[^,\\s]*),\\s*eSCL\\s*$");

    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QRegularExpressionMatch match = lineRegex.match(line);
        if (!match.hasMatch()) continue;
        const QString deviceId = "escl:" + match.captured(3);
        const QString displayName = QString("escl: %1 (%2)").arg(match.captured(1), match.captured(4));
        SettingsManager::instance().scannerValues.insert(deviceId, displayName);
        SettingsManager::instance().scannerUrls.insert(deviceId, match.captured(2));
    }
}

// The user's scanner if it was found. If the search missed it, it is tried directly (escl opens
// its URL without a search); loading its options decides whether it is there
// (getScannerCapabilities). Without a saved scanner: the first one, for this session only.
// false: no scanner at all.
bool MainWin::chooseScanner() {
    SettingsManager &settings = SettingsManager::instance();
    const QString saved = settings.savedDeviceId();
    m_probingSavedScanner = false;

    if (!saved.isEmpty()) {
        if (!settings.scannerValues.contains(saved)) {
            const QString name = settings.savedDeviceName();
            settings.scannerValues.insert(saved, name.isEmpty() ? saved : name);
            m_probingSavedScanner = true;
        }
        settings.setDeviceId(saved);
        return true;
    }
    if (settings.scannerValues.isEmpty()) return false;
    settings.setDeviceId(settings.scannerValues.firstKey());
    return true;
}

// The saved scanner was not found and does not answer either: the first found one stands in
// (not stored, the next search tries the saved one again)
bool MainWin::useStandInScanner() {
    SettingsManager &settings = SettingsManager::instance();
    m_probingSavedScanner = false;
    settings.scannerValues.remove(settings.savedDeviceId());
    if (settings.scannerValues.isEmpty()) {
        settings.setDeviceId(QString());
        return false;
    }
    settings.setDeviceId(settings.scannerValues.firstKey());
    getScannerCapabilities(settings.deviceId());
    return true;
}

// scanimage -L lines (used when airscan-discover is not installed)
void MainWin::saveScanners(const QString &saneOutput) {
    QStringList lines = saneOutput.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : std::as_const(lines)) {

        // Extract technical ID (between ` and ')
        int startId = line.indexOf('`') + 1;
        int endId = line.indexOf('\'');
        if (startId <= 0 || endId <= startId) continue;

        QString deviceId = line.mid(startId, endId - startId);  // Deivce ID
        if (deviceId.startsWith("v4l")) {
            continue; // Skip webcams
        }
        QString backend = deviceId.section(':', 0, 0);          // Backend

        // The escl backend (SANE 1.4) cannot open IPv6 URLs ("Invalid argument"); the same
        // scanner is usually also listed with its IPv4 address
        if (backend == "escl" && deviceId.contains("://[")) {
            continue;
        }

        // remove device
        QString remainingText = line.section("is a ", 1, 1).trimmed();

        // Get connection info
        QString connectionInfo;

        if (remainingText.contains("ip=")) {
            connectionInfo = remainingText.section("ip=", 1, 1).section(',', 0, 0).trimmed();
        } else if (deviceId.contains("libusb")) {
            connectionInfo = "USB " + deviceId.section("libusb:", 1, 1);
        } else if (backend == "escl") {
            // escl:http://192.168.178.31:8080 -> 192.168.178.31
            connectionInfo = QUrl(deviceId.mid(backend.length() + 1)).host();
        }
        //qDebug() << "Connetion Info: " << connectionInfo;

        // Get Display Name (trim ip)
        QString rawName = remainingText.section("ip=", 0, 0).section('?', 0, 0).trimmed();

        // 5. Zusammenbau
        QString displayName = QString("%1: %2").arg(backend, rawName.trimmed());
        if (!connectionInfo.isEmpty()) {
            displayName += QString(" (%1)").arg(connectionInfo);
        }

        SettingsManager::instance().scannerValues.insert(deviceId, displayName);
    }
}

void MainWin::getScannerCapabilities(const QString &deviceID) {
    m_scannerReady = false;
    if (deviceID.isEmpty()) {
        // e.g. only unusable devices were found: refresh must stay possible
        m_statusOverlay->hideOverlay();
        setBusy(false);
        return;
    }

    qDebug() << "Get scanner capabilities...";
    setBusy(true);
    m_statusOverlay->showMessage(tr("LOAD_SCANNER_CAPABILITIES..."));
    m_statusOverlay->raise();
    // QProcess auf dem Heap, damit er weiterläuft, wenn die Funktion endet
    QProcess *process = new QProcess(this);

    // Wir verbinden das finished-Signal DES NEUEN Prozesses
    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            // The saved scanner was tried without being found: it is really not there
            if (m_probingSavedScanner) {
                if (useStandInScanner()) return;
                m_statusOverlay->showMessage(tr("NO_SCANNERS_DETECTED."), true, 3000);
                setBusy(false);
                return;
            }
            m_statusOverlay->showMessage(tr("Scanner capabilities could not be loaded"), true, 3000);
            setBusy(false);   // refresh and choosing another scanner stay possible
            return;
        }

        QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
        QStringList lines = output.split('\n');

        // Options of the previous scanner are not valid for this one
        SettingsManager::instance().clearCapabilities();

        // Regex: Gruppe 1 = Name, Gruppe 2 = Optionen-Liste, Gruppe 3 = Default in []
        QRegularExpression optRegex("--([a-zA-Z0-9-]+)\\s+(.*?)\\s*\\[(.*?)\\]");

        // 1. as_const prevents the LIST from being copied (detachment)
        // 2. & prevents the individual STRING from being copied
        for (const QString &line : std::as_const(lines)) {
            QRegularExpressionMatch match = optRegex.match(line.trimmed());

            if (match.hasMatch()) {
                QString optName = match.captured(1);
                QString rawValues = match.captured(2).remove("dpi").trimmed();
                QString defaultValue = match.captured(3).trimmed();
                QStringList values = rawValues.split('|', Qt::SkipEmptyParts);

                // Werte säubern
                for(QString &v : values) v = v.trimmed();

                values.removeAll("*unset*");
                // airscan offers "*unset*" as default (the scanner decides): the combo could not show
                // it, so the first real value stands in ("Document" for the scan intent)
                if (defaultValue == "*unset*") {
                    defaultValue = values.contains("Document") ? QString("Document") : values.value(0);
                }

                // Option exists but cannot be used in the current device state
                if (defaultValue == "inactive") continue;

                // Zuordnung zu deinen Variablen
                // Saved values that the (new) scanner does not support are replaced by its default
                if (optName == "resolution") {
                    values = expandResolutionValues(values, defaultValue);
                    SettingsManager::instance().resolutionValues = values;
                    SettingsManager::instance().resolutionDefault = defaultValue;
                    if (!values.contains(QString::number(SettingsManager::instance().resolution()))) {
                        SettingsManager::instance().setResolution(defaultValue.toInt());
                    }
                }
                else if (optName == "mode") {
                    // Only decides what to scan with; the app's color mode stays (scannerModeFor)
                    SettingsManager::instance().colorModeValues = values;
                    SettingsManager::instance().colorModeDefault = defaultValue;
                }
                else if (optName == "scan-intent") {
                    SettingsManager::instance().intentValues = values;
                    SettingsManager::instance().intentDefault = defaultValue;
                    if (!values.contains(SettingsManager::instance().scanIntent())) {
                        SettingsManager::instance().setScanIntent(defaultValue);
                    }
                }
                else if (optName == "source") {
                    SettingsManager::instance().scanningUnitValues = values;
                    SettingsManager::instance().scanningUnitDefault = defaultValue;
                    QString scanningUnit = SettingsManager::instance().scanningUnit();
                    if (scanningUnit != "Automatic" && !values.contains(scanningUnit)) {
                        SettingsManager::instance().setScanningUnit(defaultValue);
                    }
                }
            }
        }
        m_statusOverlay->hideOverlay();
        m_probingSavedScanner = false;   // the saved scanner answered: it stays in the list
        m_scannerReady = true;
        setBusy(false);
#ifdef QT_DEBUG
        SettingsManager::instance().dump();
#endif
    });

    // scanimage not installed / not executable: finished() is never emitted in this case
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        m_statusOverlay->showMessage(tr("Scanner capabilities could not be loaded"), true, 3000);
        setBusy(false);
        process->deleteLater();
    });

    const SaneDevice::Target target =
        SaneDevice::forDevice(deviceID, SettingsManager::instance().esclUrl(deviceID));
    process->setProcessEnvironment(target.environment);
    process->start("scanimage", QStringList() << "-d" << target.device << "--help");
}

void MainWin::startScan() {
    qDebug() << "Enter startScan...";

    // -- Lock UI elements
    setBusy(true);

    m_statusOverlay->showMessage(tr("SCANNING..."));

    // -- Preparing the list and paths
    m_processedFiles.clear(); // Reset list for this scan run
    m_scanErrorOutput.clear();

    const QString colorMode = SettingsManager::instance().colorMode();
    const QString scannerMode = PageProcessing::scannerModeFor(colorMode, SettingsManager::instance().colorModeValues);
    QString resolution = QString::number(SettingsManager::instance().resolution());
    QString scanDevice = SettingsManager::instance().deviceId();
    QString source = SettingsManager::instance().scanningUnit();
    QString scanIntent = SettingsManager::instance().scanIntent();

    // "Automatic" (or nothing) means: let the scanner use its default source
    bool setSource = SettingsManager::instance().scanningUnitValues.contains(source);
    QString effectiveSource = setSource ? source : SettingsManager::instance().scanningUnitDefault;

    QString width = "210";
    QString height = "297";

    // -- Set Page Format
    QString pageFormat = SettingsManager::instance().pageFormat();
    if (pageFormat == "Standard") {
        pageFormat = getPageFormat();
    }

    if (pageFormat == "A4") {
        width = "210"; height = "297";
    } else if (pageFormat == "A5") {
        width = "148"; height = "210";
    } else if (pageFormat == "A6") {
        width = "105"; height = "148";
    } else if (pageFormat == "Letter") {
        width = "215.9"; height = "279.4";
    } else if (pageFormat == "Legal") {
        width = "215.9"; height = "355.6";
    }
    // Check orientation
    if (SettingsManager::instance().isLandscape()) {
        std::swap(width, height);
    }

    // Create temporary directory in /tmp
    QString batchDirName = "scan_job_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QDir tempDir(QDir::tempPath());
    if (!tempDir.mkpath(batchDirName)) {
        qDebug() << "Fehler: Konnte Verzeichnis nicht erstellen";
        m_statusOverlay->showMessage(tr("ERROR_CREATING_TEMP_DIR."), true, 3000);
        setBusy(false);
        return;
    }
    QString batchPath = tempDir.absoluteFilePath(batchDirName);

    // Configure QProcess
    QProcess *scanProcess = new QProcess(this);
    scanProcess->setWorkingDirectory(batchPath);

    // While scanning the scan button cancels (updateButtons keeps it usable)
    m_scanProcess = scanProcess;
    m_scanCancelled = false;
    m_scanBtn->setText(tr("Cancel"));
    updateButtons();

    QString program = "scanimage";
    QStringList arguments;
    // escl scanners through airscan: escl takes only the first page of a feeder job (SaneDevice)
    const SaneDevice::Target target =
        SaneDevice::forDevice(scanDevice, SettingsManager::instance().esclUrl(scanDevice));
    scanProcess->setProcessEnvironment(target.environment);
    arguments << "--device-name" << target.device;
    // Only pass options the scanner reported, otherwise scanimage aborts
    if (!scannerMode.isEmpty()) {
        arguments << "--mode" << scannerMode;
    }
    // Remembered with every page: the PDF page size follows from pixels and resolution
    int scanDpi = 0;
    if (!SettingsManager::instance().resolutionValues.isEmpty()) {
        arguments << "--resolution" << resolution;
        scanDpi = resolution.toInt();
    }
    if (setSource) {
        arguments << "--source" << source;
    }
    if (SettingsManager::instance().intentValues.contains(scanIntent)) {
        arguments << "--scan-intent" << scanIntent;
    }
    arguments << "-x" << width;
    arguments << "-y" << height;
    //  When ADF is selected bach mode with more than one page
    if (isAdfSource(effectiveSource)) {
        arguments << "--batch=page_%03d.png";
    } else {
        // Only one page for flatbed
        arguments << "--batch=page_001.png";
        arguments << "--batch-count=1";
    }
    arguments << "--format=png";

    // 4. Timer für die Live-Anzeige (alle 1,5 Sekunden prüfen)
    QTimer *progressTimer = new QTimer(this);
    connect(progressTimer, &QTimer::timeout, this, [this, batchPath, scanDpi, colorMode]() {
        QDir dir(batchPath);
        QStringList currentFiles = dir.entryList(QStringList() << "page_*.png", QDir::Files, QDir::Name);

        for (const QString &fileName : std::as_const(currentFiles)) {
            // Nur verarbeiten, wenn wir die Datei noch nicht im UI haben
            if (!m_processedFiles.contains(fileName)) {
                QString fullPath = dir.absoluteFilePath(fileName);

                // CRASH-SCHUTZ: Wir kopieren die Datei, bevor wir sie laden,
                // damit wir scanimage nicht beim Schreiben stören.
                QString tempCopy = fullPath + ".tmp";
                if (QFile::copy(fullPath, tempCopy)) {
                    QImage img(tempCopy);
                    // Prüfen, ob das Bild valide und nicht leer ist
                    if (!img.isNull() && img.width() > 0) {
                        PageProcessing::convertScannedPage(fullPath, colorMode);   // the page is complete now
                        this->addPageToList(fullPath, scanDpi); // Hier wird das Item im UI erstellt
                        m_processedFiles.append(fileName);
                    }
                    QFile::remove(tempCopy); // Temporäre Kopie sofort löschen
                }
            }
        }
    });

    // 5. Fehler-Logging für den Prozess
    connect(scanProcess, &QProcess::errorOccurred, this, [this, scanProcess, progressTimer](QProcess::ProcessError error) {
        QMetaEnum metaEnum = QMetaEnum::fromType<QProcess::ProcessError>();
        QString errorName = metaEnum.valueToKey(error);
        qDebug() << "PROCESS ERROR:" << errorName << scanProcess->errorString();
        m_statusOverlay->showMessage(tr("ERROR:") + "<br>" + errorName, true, 6000);

        // finished() is not emitted if the process could not be started, so clean up here
        if (error == QProcess::FailedToStart) {
            progressTimer->stop();
            progressTimer->deleteLater();
            endScan();
            scanProcess->deleteLater();
        }
    });

    // Read messages from scanner
    connect(scanProcess, &QProcess::readyReadStandardError, this, [this, scanProcess]() {
        QString message = QString::fromLocal8Bit(scanProcess->readAllStandardError()).trimmed();
        qDebug() << "SCANNER STATUS:" << message;
        // Keep the last message to show it if the scan fails
        if (!message.isEmpty()) m_scanErrorOutput = message.section('\n', -1);
    });

    // 6. Abschluss-Logik
    connect(scanProcess, &QProcess::finished, this, [this, batchPath, scanDpi, colorMode, scanProcess, progressTimer](int exitCode, QProcess::ExitStatus exitStatus) {
        progressTimer->stop();
        progressTimer->deleteLater();

        // Letzter Check: Dateien einlesen, die der Timer evtl. am Ende verpasst hat.
        // After a cancel the last page may be half written: it is dropped and deleted.
        QDir dir(batchPath);
        QStringList finalFiles = dir.entryList(QStringList() << "page_*.png", QDir::Files, QDir::Name);
        for (const QString &fileName : std::as_const(finalFiles)) {
            if (!m_processedFiles.contains(fileName)) {
                const QString fullPath = dir.absoluteFilePath(fileName);
                if (QImage(fullPath).isNull()) {
                    QFile::remove(fullPath);
                    continue;
                }
                PageProcessing::convertScannedPage(fullPath, colorMode);
                this->addPageToList(fullPath, scanDpi);
                m_processedFiles.append(fileName);
            }
        }

        // UI aufräumen
        const bool cancelled = m_scanCancelled;
        endScan();
        m_statusOverlay->hideOverlay();

        // Falls es ein echter Fehler war (nicht EOF = 5 / ADF leer = 7 nach gescannten Seiten)
        bool feederEmpty = (exitCode == 5 || exitCode == 7) && !m_processedFiles.isEmpty();
        if (cancelled) {
            // scanimage ends with an error code after a cancel: no error for the user
            m_statusOverlay->showMessage(tr("Scan cancelled"), false, 3000);
        } else if (exitStatus != QProcess::NormalExit || (exitCode != 0 && !feederEmpty)) {
            qDebug() << "Scan abgebrochen mit Code:" << exitCode;
            m_statusOverlay->showMessage(tr("SCAN_FAILED.") + "<br>" + breakableHtml(m_scanErrorOutput), true, 6000);
        }

        scanProcess->deleteLater();
        qDebug() << "Scan-Vorgang beendet. Verzeichnis:" << batchPath;
    });

    // Start!
    progressTimer->start(1500);
    scanProcess->start(program, arguments);
}

// Scan button and Esc while scanning. scanimage cancels on the first SIGTERM (sane_cancel: the
// scanner is told, escl deletes the scan job); if it hangs, it is killed after 5 s. Pages that
// are complete already stay.
void MainWin::cancelScan() {
    if (!m_scanProcess || m_scanCancelled) return;
    m_scanCancelled = true;
    updateButtons();   // one cancel is enough
    m_statusOverlay->showMessage(tr("Cancelling scan..."));
    QProcess *process = m_scanProcess;
    process->terminate();
    QTimer::singleShot(5000, process, [process]() {
        if (process->state() != QProcess::NotRunning) process->kill();
    });
}

// Back to normal after a scan (finished, failed to start or cancelled)
void MainWin::endScan() {
    m_scanProcess = nullptr;
    m_scanCancelled = false;
    m_scanBtn->setText(tr("SCAN"));
    setBusy(false);
}

void MainWin::addPageToList(const QString &filePath, int dpi) {
    // The new page becomes the current one, the preview shows it (showCurrentPage)
    ui->pagesListWidget->addPage(filePath, dpi);
}

void MainWin::refreshScanners() {
    searchForScanners();
}

// ====== Pages ======

// Selected or all pages in list order. Without OCR the PDF is written here (QPdfWriter);
// with OCR (later) tesseract writes it from the same images instead.
void MainWin::savePdf(bool selectedOnly) {
    const QList<PageListWidget::Page> pages = ui->pagesListWidget->pages(selectedOnly);
    if (pages.isEmpty()) return;

    SettingsManager &settings = SettingsManager::instance();
    const QString name = "Scan_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmm") + ".pdf";
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save PDF"),
                                                    QDir(settings.lastSaveFolder()).filePath(name),
                                                    tr("PDF files (*.pdf)"));
    if (filePath.isEmpty()) return;
    if (!filePath.endsWith(".pdf", Qt::CaseInsensitive)) filePath += ".pdf";
    settings.setLastSaveFolder(QFileInfo(filePath).absolutePath());

    setBusy(true);
    m_statusOverlay->showMessage(tr("Saving PDF..."));
    // Writing blocks the window: let the message appear first
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    const bool saved = writePdf(filePath, pages);
    setBusy(false);

    if (saved) {
        m_statusOverlay->showMessage(tr("PDF saved: %1").arg(QFileInfo(filePath).fileName()), false, 3000);
    } else {
        QFile::remove(filePath);   // no half written file
        m_statusOverlay->showMessage(tr("PDF could not be saved"), true, 3000);
    }
}

void MainWin::rotatePages() {
    QApplication::setOverrideCursor(Qt::WaitCursor);   // large scans take a moment
    ui->pagesListWidget->rotateSelectedPages();
    QApplication::restoreOverrideCursor();
}

// The image files are deleted too, so ask first
void MainWin::clearPages() {
    if (ui->pagesListWidget->count() == 0) return;
    QMessageBox box(QMessageBox::Question, tr("Clear"),
                    tr("Remove all pages? The scanned images are deleted."), QMessageBox::NoButton, this);
    QPushButton *remove = box.addButton(tr("Remove"), QMessageBox::DestructiveRole);
    box.addButton(tr("Cancel"), QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() == remove) ui->pagesListWidget->removeAllPages();
}

// ====== Button states ======

// Search, loading the scanner options and scanning lock the toolbar
void MainWin::setBusy(bool busy) {
    m_busy = busy;
    updateButtons();
}

// The one place that decides which buttons are usable (page buttons follow with the pages)
void MainWin::updateButtons() {
    const bool hasScanners = !SettingsManager::instance().scannerValues.isEmpty();
    m_refreshBtn->setEnabled(!m_busy);
    m_coffeeBtn->setEnabled(!m_busy);
    m_helpBtn->setEnabled(!m_busy);
    m_settingsBtn->setEnabled(!m_busy && hasScanners);
    // While scanning it is the cancel button (until it was clicked)
    m_scanBtn->setEnabled(m_scanProcess ? !m_scanCancelled : !m_busy && m_scannerReady);

    const bool hasPages = ui->pagesListWidget->count() > 0;
    const bool hasSelection = ui->pagesListWidget->hasSelection();
    m_rotateBtn->setEnabled(!m_busy && hasSelection);
    m_saveSelBtn->setEnabled(!m_busy && hasSelection);
    m_saveAllBtn->setEnabled(!m_busy && hasPages);
    m_clearBtn->setEnabled(!m_busy && hasPages);
}

// ====== Events ======

void MainWin::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (m_statusOverlay) m_statusOverlay->resize(this->size());
}

void MainWin::changeEvent(QEvent *event) {
    switch (event->type()) {
    case QEvent::PaletteChange:
    case QEvent::ApplicationPaletteChange:
        if (m_statusOverlay) applySurfaceColors();  // not before initializeForm() has run
        break;
    default:
        break;
    }
    QMainWindow::changeEvent(event);
}

void MainWin::closeEvent(QCloseEvent *event) {
    SettingsManager::instance().saveWindowLayout(saveGeometry(), ui->splitter->saveState());
    QMainWindow::closeEvent(event);
}

MainWin::~MainWin()
{
    delete ui;
}

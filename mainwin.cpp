#include "mainwin.h"
#include "./ui_mainwin.h"
#include "settingsmanager.h"
#include <QDir>
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

void MainWin::onFilelistTableViewClicked(const QModelIndex &index) {
    // Show the clicked page in the large preview
    QString filePath = index.data(Qt::UserRole).toString();
    if (!filePath.isEmpty())
        ui->previewGraphicsView->setImage(filePath);
}

// ====== Scanner functions ======

void MainWin::searchForScanners() {
    m_statusOverlay->showMessage(tr("SEARCH_SCANNERS..."));

    QProcess *process = new QProcess(this);

    // scanimage not installed / not executable: finished() is never emitted in this case
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        m_statusOverlay->showMessage(tr("EROOR_STARTING_SEARCH."), true, 3000);
        m_refreshBtn->setEnabled(true);
        process->deleteLater();
    });

    connect(process, &QProcess::finished, this, [this, process]() {
        // A) Wurde der Prozess überhaupt korrekt beendet?
        if (process->exitStatus() != QProcess::NormalExit) {
            m_statusOverlay->showMessage(tr("EROOR_STARTING_SEARCH."), true, 3000);
            m_refreshBtn->setEnabled(true);
            process->deleteLater();
            return;
        }

        QString output = QString::fromLocal8Bit(process->readAllStandardOutput());

        // B) Ist der Output technisch leer?
        if (output.trimmed().isEmpty()) {
            m_statusOverlay->showMessage(tr("NO_SCANNER_HARDWARE_FOUND."), true, 3000);
            m_refreshBtn->setEnabled(true);
            process->deleteLater();
            return;
        }

        // --- VERBESSERTE PRÜFUNG C ---
        // Wir filtern den Output gedanklich vor:
        QStringList lines = output.split('\n');
        bool echterScannerGefunden = false;

        for (const QString &line : std::as_const(lines)) {
            // Enthält die Zeile ein Device UND ist es keine Webcam (v4l)?
            if (line.contains("device") && !line.contains("v4l")) {
                echterScannerGefunden = true;
                break;
            }
        }

        if (!echterScannerGefunden) {
            m_statusOverlay->showMessage(tr("NO_SCANNERS_DETECTED."), true, 3000);
            m_refreshBtn->setEnabled(true);
            process->deleteLater();
            return;
        }
        m_statusOverlay->hideOverlay();
        // Only when you have passed all the tests can you start parsing...
        this->saveScanners(output);
        process->deleteLater();
        m_statusOverlay->showMessage(tr("LOAD_SCANNER_CAPABILITIES..."));

        QTimer::singleShot(10, this, [this]() {
            this->getScannerCapabilities(SettingsManager::instance().deviceId());
        });
    });

    process->start("scanimage", QStringList() << "-L");
}

void MainWin::saveScanners(const QString &saneOutput) {
    // Forget scanners from a previous search
    SettingsManager::instance().scannerValues.clear();

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
    // Get first entry if no deviceID is avaiable or the saved scanner is no longer present
    if(!SettingsManager::instance().scannerValues.contains(SettingsManager::instance().deviceId())) {
        if (!SettingsManager::instance().scannerValues.isEmpty()) {
            SettingsManager::instance().setDeviceId(SettingsManager::instance().scannerValues.firstKey());
        } else {
            // Fallback, falls die Map leer ist
            // firstScannerKey = "Kein Scanner gefunden";
            qDebug() << "Warnung: scanners list is empty!";
        }
    }
}

void MainWin::getScannerCapabilities(const QString &deviceID) {
if(deviceID.isEmpty()) return;

    qDebug() << "Get scanner capabilities...";
    m_scanBtn->setEnabled(false);
    m_settingsBtn->setEnabled(false);
    m_refreshBtn->setEnabled(false);
    m_statusOverlay->showMessage(tr("LOAD_SCANNER_CAPABILITIES..."));
    m_statusOverlay->raise();
    // QProcess auf dem Heap, damit er weiterläuft, wenn die Funktion endet
    QProcess *process = new QProcess(this);

    // Wir verbinden das finished-Signal DES NEUEN Prozesses
    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();
        // Allow refresh and choosing another scanner even if loading fails
        m_settingsBtn->setEnabled(true);
        m_refreshBtn->setEnabled(true);

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            m_statusOverlay->showMessage(tr("Scanner capabilities could not be loaded"), true, 3000);
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
                    SettingsManager::instance().colorModeValues = values;
                    SettingsManager::instance().colorModeDefault = defaultValue;
                    if (!values.contains(SettingsManager::instance().colorMode())) {
                        SettingsManager::instance().setColorMode(defaultValue);
                    }
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
        m_scanBtn->setEnabled(true);
#ifdef QT_DEBUG
        SettingsManager::instance().dump();
#endif
    });

    // scanimage not installed / not executable: finished() is never emitted in this case
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        m_statusOverlay->showMessage(tr("Scanner capabilities could not be loaded"), true, 3000);
        m_refreshBtn->setEnabled(true);
        process->deleteLater();
    });

    process->start("scanimage", QStringList() << "-d" << deviceID << "--help");
}

void MainWin::startScan() {
    qDebug() << "Enter startScan...";

    // -- Lock UI elements
    m_scanBtn->setEnabled(false);
    m_settingsBtn->setEnabled(false);
    m_refreshBtn->setEnabled(false);

    m_statusOverlay->showMessage(tr("SCANNING..."));

    // -- Preparing the list and paths
    m_processedFiles.clear(); // Reset list for this scan run
    m_scanErrorOutput.clear();

    QString mode = SettingsManager::instance().colorMode();
    QString realMode = (mode == "Lineart") ? "Gray" : mode;
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
        m_scanBtn->setEnabled(true);
        m_settingsBtn->setEnabled(true);
        m_refreshBtn->setEnabled(true);
        return;
    }
    QString batchPath = tempDir.absoluteFilePath(batchDirName);

    // Configure QProcess
    QProcess *scanProcess = new QProcess(this);
    scanProcess->setWorkingDirectory(batchPath);

    QString program = "scanimage";
    QStringList arguments;
    arguments << "--device-name" << scanDevice;
    // Only pass options the scanner reported, otherwise scanimage aborts
    if (!SettingsManager::instance().colorModeValues.isEmpty()) {
        arguments << "--mode" << realMode;
    }
    if (!SettingsManager::instance().resolutionValues.isEmpty()) {
        arguments << "--resolution" << resolution;
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
    connect(progressTimer, &QTimer::timeout, this, [this, batchPath]() {
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
                        this->addPageToList(fullPath); // Hier wird das Item im UI erstellt
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
            m_scanBtn->setEnabled(true);
            m_settingsBtn->setEnabled(true);
            m_refreshBtn->setEnabled(true);
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
    connect(scanProcess, &QProcess::finished, this, [this, batchPath, scanProcess, progressTimer](int exitCode, QProcess::ExitStatus exitStatus) {
        progressTimer->stop();
        progressTimer->deleteLater();

        // Letzter Check: Dateien einlesen, die der Timer evtl. am Ende verpasst hat
        QDir dir(batchPath);
        QStringList finalFiles = dir.entryList(QStringList() << "page_*.png", QDir::Files, QDir::Name);
        for (const QString &fileName : std::as_const(finalFiles)) {
            if (!m_processedFiles.contains(fileName)) {
                this->addPageToList(dir.absoluteFilePath(fileName));
                m_processedFiles.append(fileName);
            }
        }

        // UI aufräumen
        m_scanBtn->setEnabled(true);
        m_settingsBtn->setEnabled(true);
        m_refreshBtn->setEnabled(true);
        m_statusOverlay->hideOverlay();

        // Falls es ein echter Fehler war (nicht EOF = 5 / ADF leer = 7 nach gescannten Seiten)
        bool feederEmpty = (exitCode == 5 || exitCode == 7) && !m_processedFiles.isEmpty();
        if (exitStatus != QProcess::NormalExit || (exitCode != 0 && !feederEmpty)) {
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

void MainWin::addPageToList(const QString &filePath) {
    if (!ui->pagesListWidget->addPage(filePath)) return;
    // Show the new page in the large preview right away
    ui->previewGraphicsView->setImage(filePath);
}

void MainWin::refreshScanners() {
    m_scanBtn->setEnabled(false);
    m_settingsBtn->setEnabled(false);
    m_refreshBtn->setEnabled(false);
    searchForScanners();
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

MainWin::~MainWin()
{
    delete ui;
}

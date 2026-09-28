#include "mainwin.h"
#include "./ui_mainwin.h"
#include "toolbarm.h"
#include "settingsmanager.h"
#include <QDir>
#include <QThread>

MainWin::MainWin(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWin)
{
    ui->setupUi(this);
    initializeForm();
    m_statusOverlay = new StatusOverlay(this);
    m_settingsOverlay = new SettingsOverlay(nullptr);
    m_settingsOverlay->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

    initializeConnections();
    QTimer::singleShot(0, this, [this]() {
        int breite = SettingsManager::instance().splitterWidth();
        int leftWidth = 180;
        int rightWidth = ui->splitter->width() - leftWidth;
        ui->splitter->setSizes({leftWidth, rightWidth});
        ui->pagesListWidget->setMinimumWidth(180);  // Prevent the list from collapsing to 0
    });
    searchForScanners();
}

// ====== Initializing / Settings  ======

void MainWin::initializeForm() {
    // --- Create Buttons and objects ---
    ui->toolBarFrame->addButton("scanBtn",tr("SCAN"),"");
    m_scanBtn = ui->toolBarFrame->findChild<QToolButton*>("scanBtn");
    m_scanBtn->setEnabled(false);

    ui->toolBarFrame->addButton("settingsBtn","",":icons/settings");
    m_settingsBtn = ui->toolBarFrame->findChild<QToolButton*>("settingsBtn");
    m_settingsBtn->setEnabled(false);

    ui->toolBarFrame->addButton("refreshBtn","",":icons/refresh");
    m_refreshBtn = ui->toolBarFrame->findChild<QToolButton*>("refreshBtn");
    m_refreshBtn->setEnabled(false);

    ui->toolBarFrame->addSpacer();  // Spacer for align buttons right

    ui->toolBarFrame->addButton("rotateBtn","",":icons/document_rotate");
    m_rotateBtn = ui->toolBarFrame->findChild<QToolButton*>("rotateBtn");
    m_rotateBtn->setEnabled(false);

    ui->toolBarFrame->addButton("fakeDplxBtn",tr("FAKE_DUPLEX"),"");
    m_fakeDplxBtn = ui->toolBarFrame->findChild<QToolButton*>("fakeDplxBtn");
    m_fakeDplxBtn->setEnabled(false);

    ui->toolBarFrame->addButton("saveSelBtn",tr("SAVE_SELECTED"),":icons/upload",true,true);
    m_saveSelBtn = ui->toolBarFrame->findChild<QToolButton*>("saveSelBtn");
    m_saveSelBtn->setEnabled(false);

    ui->toolBarFrame->addButton("saveAllBtn",tr("SAVE_ALL"),":icons/upload",true,true);
    m_saveAllBtn = ui->toolBarFrame->findChild<QToolButton*>("saveAllBtn");
    m_saveAllBtn->setEnabled(false);

    ui->toolBarFrame->addButton("clearBtn",tr("CLEAR"),"");
    m_clearBtn = ui->toolBarFrame->findChild<QToolButton*>("clearBtn");
    m_clearBtn->setEnabled(false);

    auto* scene = new QGraphicsScene(this);
    ui->previewGraphicsView->setScene(scene);

    // Page List View
    // int plwHeight = ui->pagesListWidget->height();
    // ui->pagesListWidget->setGeometry(0,0,plwHeight,150);
    ui->pagesListWidget->setViewMode(QListView::IconMode);  // Activate gallery mode
    //ui->pagesListWidget->setMovement(QListView::Static);
    ui->pagesListWidget->setSpacing(10);   // Distance between pages
    ui->pagesListWidget->setResizeMode(QListView::Adjust);  // Align the text nicely in the center

    // Das Item, das wir später mit dem Bild füllen
    QPixmap newImage(":/icons/scan_document.svg");

    m_previewScene = new QGraphicsScene(this);
    ui->previewGraphicsView->setScene(m_previewScene);
}

void MainWin::updateListIconSize() {
    int w = ui->pagesListWidget->viewport()->width() - 20;
    int h = static_cast<int>(w * 1.41); // A4 Verhältnis

    QSize newSize(w, h);
    ui->pagesListWidget->setIconSize(newSize);

    // Wir müssen jedes einzelne Item "anstoßen"
    for (int i = 0; i < ui->pagesListWidget->count(); ++i) {
        QListWidgetItem *item = ui->pagesListWidget->item(i);
        // Wir setzen den sizeHint manuell auf die neue Größe + Platz für Text
        item->setSizeHint(QSize(w - 10, h + 30));
    }
    ui->pagesListWidget->doItemsLayout(); // Layout-Engine zum Rechnen zwingen
}

// void MainWin::updateListIconSize() {
//     // 1. Rekursions-Schutz: Timer kurz stoppen, falls vorhanden,
//     // oder Signale blockieren, damit wir uns nicht selbst aufrufen.
//     ui->pagesListWidget->blockSignals(true);

//     int w = ui->pagesListWidget->viewport()->width() - 25;
//     if (w < 40) return; // Zu klein, abbrechen

//     int h = static_cast<int>(w * 1.41);
//     QSize newSize(w, h);

//     // Nur updaten, wenn sich die Größe wirklich signifikant geändert hat
//     if (ui->pagesListWidget->iconSize() == newSize) {
//         ui->pagesListWidget->blockSignals(false);
//         return;
//     }

//     ui->pagesListWidget->setIconSize(newSize);

//     for (int i = 0; i < ui->pagesListWidget->count(); ++i) {
//         QListWidgetItem *item = ui->pagesListWidget->item(i);

//         // Wir setzen NUR den sizeHint.
//         // Das Icon lassen wir so, wie es ist (highRes),
//         // Qt skaliert es intern beim Zeichnen selbst runter!
//         item->setSizeHint(QSize(w, h + 35));
//     }

//     ui->pagesListWidget->doItemsLayout();
//     ui->pagesListWidget->blockSignals(false);
// }

void MainWin::initializeConnections() {
    //qDebug() << "Initialize Connects";
    connect(m_scanBtn, &QToolButton::clicked, this, &MainWin::startScan);
    connect(m_settingsBtn, &QToolButton::clicked, this, &MainWin::showSettingsOverlay);
    connect(m_refreshBtn, &QToolButton::clicked, this, &MainWin::refreshScanners);
    connect(ui->pagesListWidget, &QListView::clicked, this, &::MainWin::onFilelistTableViewClicked);
    connect(ui->splitter, &QSplitter::splitterMoved, this, [this](int pos, int index) {
        updateListIconSize();
    });
    connect(m_settingsOverlay, &SettingsOverlay::scannerChanged,
            this, &MainWin::getScannerCapabilities);
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

void MainWin::updatePreview(const QString &imagePath) {
    // // 1. Scene initialisieren, falls noch nicht geschehen
    // if (m_previewScene) {
    //     m_previewScene = new QGraphicsScene(this);
    //     ui->graphicsView->setScene(m_previewScene);
    // }

    // Load Picture
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        qDebug() << "Fehler: Bild konnte nicht geladen werden!";
        return;
    }

    // Clear Scene
    m_previewScene->clear();

    // Put Pixmap to Scene
    m_previewItem = m_previewScene->addPixmap(pixmap);

    // 5. Ansicht anpassen (optional, damit das Bild ganz reinpasst)
    ui->previewGraphicsView->fitInView(m_previewScene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWin::onFilelistTableViewClicked(const QModelIndex &index) {
    // Show the clicked page in the large preview
    QString filePath = index.data(Qt::UserRole).toString();
    if (!filePath.isEmpty())
        updatePreview(filePath);
}

// ====== Scanner functions ======

void MainWin::searchForScanners() {
    m_statusOverlay->showMessage(tr("SEARCH_SCANNERS..."));

    QProcess *process = new QProcess(this);
    process->start("scanimage", QStringList() << "-L");

    connect(process, &QProcess::finished, this, [this, process]() {
        // A) Wurde der Prozess überhaupt korrekt beendet?
        if (process->exitStatus() != QProcess::NormalExit) {
            m_statusOverlay->showMessage(tr("EROOR_STARTING_SEARCH."), true, 3000);
            process->deleteLater();
            return;
        }

        QString output = QString::fromLocal8Bit(process->readAllStandardOutput());

        // B) Ist der Output technisch leer?
        if (output.trimmed().isEmpty()) {
            m_statusOverlay->showMessage(tr("NO_SCANNER_HARDWARE_FOUND."), true, 3000);
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
            process->deleteLater();
            return;
        }
        m_statusOverlay->hide();
        // Only when you have passed all the tests can you start parsing...
        this->saveScanners(output);
        process->deleteLater();
        m_statusOverlay->showMessage(tr("LOAD_SCANNER_CAPABILITIES..."));
        QCoreApplication::processEvents();

        QTimer::singleShot(10, this, [this]() {
            this->getScannerCapabilities(SettingsManager::instance().deviceId());
        });
    });
}

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

        // remove device
        QString remainingText = line.section("is a ", 1, 1).trimmed();

        // Get connection info
        QString connectionInfo;

        if (remainingText.contains("ip=")) {
            connectionInfo = remainingText.section("ip=", 1, 1).section(',', 0, 0).trimmed();
        } else if (deviceId.contains("libusb")) {
            connectionInfo = "USB " + deviceId.section("libusb:", 1, 1);
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
    // Get first entry if no deviceID is avaiable
    if(SettingsManager::instance().deviceId().isEmpty()) {
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
    connect(process, &QProcess::finished, this, [this, process]() {
        QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
        QStringList lines = output.split('\n');

        // Variablen für die Ergebnisse
        QStringList resValues, modeValues, intentValues, sourceValues;
        QString resDefault, modeDefault, intentDefault, sourceDefault;

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

                // Zuordnung zu deinen Variablen
                if (optName == "resolution") {
                    SettingsManager::instance().resolutionValues = values;
                    SettingsManager::instance().resolutionDefault = defaultValue;
                    if (!SettingsManager::instance().resolution()) {
                        SettingsManager::instance().setResolution(defaultValue.toInt());
                    }
                }
                else if (optName == "mode") {
                    SettingsManager::instance().colorModeValues = values;
                    SettingsManager::instance().colorModeDefault = defaultValue;
                    if (SettingsManager::instance().colorMode() == "") {
                        SettingsManager::instance().setColorMode(defaultValue);
                    }
                }
                else if (optName == "scan-intent") {
                    SettingsManager::instance().intentValues = values;
                    SettingsManager::instance().intentDefault = defaultValue;
                }
                else if (optName == "source") {
                    SettingsManager::instance().scanningUnitValues = values;
                    SettingsManager::instance().scanningUnitDefault = defaultValue;
                    if (SettingsManager::instance().scanningUnit() == "") {
                        SettingsManager::instance().setScanningUnit(defaultValue);
                    }
                }
            }
        }
        m_statusOverlay->hide();
        m_scanBtn->setEnabled(true);
        m_settingsBtn->setEnabled(true);
        m_refreshBtn->setEnabled(true);
        SettingsManager::instance().dump();
    });

    connect(process, &QProcess::errorOccurred, this, [this, process]() {
        m_statusOverlay->showMessage(tr("Scanner capabilities could not be loaded"), true, 3000);
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

    QString mode = SettingsManager::instance().colorMode();
    QString realMode = (mode == "Lineart") ? "Gray" : mode;
    QString resolution = QString::number(SettingsManager::instance().resolution());
    QString scanDevice = SettingsManager::instance().deviceId();
    QString source = SettingsManager::instance().scanningUnit();

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
        return;
    }
    QString batchPath = tempDir.absoluteFilePath(batchDirName);

    // Configure QProcess
    QProcess *scanProcess = new QProcess(this);
    scanProcess->setWorkingDirectory(batchPath);

    QString program = "scanimage";
    QStringList arguments;
    arguments << "--device-name" << scanDevice;
    arguments << "--mode" << realMode;
    arguments << "--resolution" << resolution;
    arguments << "--source" << source;
    arguments << "-x" << width;
    arguments << "-y" << height;
    //  When ADF is selected bach mode with more than one page
    if (source == "ADF") {
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
    connect(scanProcess, &QProcess::errorOccurred, this, [this, scanProcess](QProcess::ProcessError error) {
        QMetaEnum metaEnum = QMetaEnum::fromType<QProcess::ProcessError>();
        QString errorName = metaEnum.valueToKey(error);
        qDebug() << "PROCESS ERROR:" << errorName << scanProcess->errorString();
        m_statusOverlay->showMessage(tr("ERROR:") + "<br>" + errorName, true, 6000);
    });

    // Read messages from scanner
    connect(scanProcess, &QProcess::readyReadStandardError, this, [scanProcess]() {
        qDebug() << "SCANNER STATUS:" << scanProcess->readAllStandardError();
    });

    // 6. Abschluss-Logik
    connect(scanProcess, &QProcess::finished, this, [this, batchPath, scanProcess, progressTimer](int exitCode) {
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

        // Falls es ein echter Fehler war (nicht ADF leer = 5)
        if (exitCode != 0 && exitCode != 5) {
            qDebug() << "Scan abgebrochen mit Code:" << exitCode;
        }

        // UI aufräumen
        m_scanBtn->setEnabled(true);
        m_settingsBtn->setEnabled(true);
        m_refreshBtn->setEnabled(true);
        m_statusOverlay->hide();

        scanProcess->deleteLater();
        qDebug() << "Scan-Vorgang beendet. Verzeichnis:" << batchPath;
    });

    // Start!
    progressTimer->start(1500);
    scanProcess->start(program, arguments);
}

void MainWin::addPageToList(const QString &filePath) {
    QImage img(filePath);
    if (img.isNull()) return;

    // Thumbnail
    QIcon icon(QPixmap::fromImage(img).scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QString itemName = tr("PAGE %1").arg(ui->pagesListWidget->count() + 1);

    QListWidgetItem *item = new QListWidgetItem(icon, itemName);
    item->setData(Qt::UserRole, filePath);

    QPixmap highRes(filePath);
    item->setIcon(QIcon(highRes.scaled(1000, 1414, Qt::KeepAspectRatio, Qt::SmoothTransformation)));

    ui->pagesListWidget->addItem(item);

    // Sofort in der großen Vorschau anzeigen
    updatePreview(filePath);

    // Optional: Automatisch zum neuesten Item scrollen
    ui->pagesListWidget->scrollToBottom();
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
    QTimer::singleShot(0, this, &MainWin::updateListIconSize);  // Calculate Icon Size
    if (m_statusOverlay) m_statusOverlay->resize(this->size());
}

MainWin::~MainWin()
{
    delete ui;
}

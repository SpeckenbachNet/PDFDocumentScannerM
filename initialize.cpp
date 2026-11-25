#include "mainwin.h"
#include "./ui_mainwin.h"
#include "toolbarm.h"
#include "uicolors.h"
#include "helppage.h"
#include "settingsmanager.h"
#include <QDesktopServices>
#include <QShortcut>

namespace {

// Toolbar look: a toolbar that lies deeper than the window, filled buttons without border
// (dark values measured on Breeze Dark). Derived from Window/WindowText only, so it looks
// the same with every palette (KDE, gtk3, Fusion, ...)
void applyToolbarStyle(ToolbarM *toolbar) {
    ToolbarM::ShadeFactors dark;
    dark.toolbarBackground = UiColors::surfaceShade;
    dark.toolbarDarkEdge   = 0.035;
    dark.toolbarLightEdge  = 0.12;
    dark.buttonFromWindow  = true;
    dark.buttonBackground  = 0.043;
    dark.buttonHover       = 0.085;
    dark.buttonPressed     = -0.115;
    dark.buttonBorder      = 0.0;
    dark.buttonBorderOff   = 0.0;
    dark.separator         = 0.085;

    // Light: the buttons need a border, otherwise they vanish on the bright toolbar
    ToolbarM::ShadeFactors light;
    light.toolbarBackground = UiColors::surfaceShade;
    light.toolbarDarkEdge   = 0.15;
    light.toolbarLightEdge  = 0.06;
    light.buttonFromWindow  = true;
    light.buttonBackground  = -0.05;
    light.buttonHover       = 0.06;
    light.buttonPressed     = 0.12;
    light.buttonBorder      = 0.18;
    light.buttonBorderOff   = 0.10;
    light.separator         = 0.25;

    toolbar->setShadeFactors(dark, light);
}

}

// ====== Toolbar ======

void MainWin::initializeToolBars() {
    applyToolbarStyle(ui->toolBarFrame);
    applyToolbarStyle(ui->helpPage->toolbar());   // the help page brings its own toolbar

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

    ui->toolBarFrame->addSpacer();  // Spacer

    ui->toolBarFrame->addButton("rotateBtn","",":icons/rotate");
    m_rotateBtn = ui->toolBarFrame->findChild<QToolButton*>("rotateBtn");
    m_rotateBtn->setToolTip(tr("Rotate the selected pages 90° clockwise"));
    m_rotateBtn->setEnabled(false);

    ui->toolBarFrame->addButton("fakeDplxBtn",tr("FAKE_DUPLEX"),"");
    m_fakeDplxBtn = ui->toolBarFrame->findChild<QToolButton*>("fakeDplxBtn");
    m_fakeDplxBtn->setEnabled(false);

    ui->toolBarFrame->addButton("saveSelBtn",tr("SAVE_SELECTED"),":icons/save",true,true);
    m_saveSelBtn = ui->toolBarFrame->findChild<QToolButton*>("saveSelBtn");
    m_saveSelBtn->setEnabled(false);

    ui->toolBarFrame->addButton("saveAllBtn",tr("SAVE_ALL"),":icons/save_all",true,true);
    m_saveAllBtn = ui->toolBarFrame->findChild<QToolButton*>("saveAllBtn");
    m_saveAllBtn->setEnabled(false);

    ui->toolBarFrame->addButton("clearBtn",tr("CLEAR"),"");
    m_clearBtn = ui->toolBarFrame->findChild<QToolButton*>("clearBtn");
    m_clearBtn->setEnabled(false);

    ui->toolBarFrame->addSpacer();  // Spacer

    // Update: only shown when a newer version is out (UpdateChecker), in the accent color
    m_updateBtn = ui->toolBarFrame->addButton("updateBtn", tr("Update"), ":icons/update");
    m_updateBtn->hide();
    styleUpdateButton();

    m_coffeeBtn = ui->toolBarFrame->addButton("coffeeBtn", "", ":icons/coffee");
    m_coffeeBtn->setToolTip(tr("Buy me a coffee – support PDFDocumentScanner via PayPal"));
    ui->toolBarFrame->setButtonColor(m_coffeeBtn, QColor(), QColor(245, 166, 35));
    m_coffeeBtn->setEnabled(false);
    m_helpBtn = ui->toolBarFrame->addButton("helpBtn", "", ":icons/help");
    m_helpBtn->setToolTip(tr("Help"));
    m_helpBtn->setEnabled(false);

}

// ====== Form ======

void MainWin::initializeForm() {
    // Page list (PageListWidget) and preview (PreviewView) set themselves up

    // Overlays
    m_statusOverlay = new StatusOverlay(this);
    // Loading the options of a scanner often takes a fraction of a second: no overlay for that
    m_capabilitiesTimer = new QTimer(this);
    m_capabilitiesTimer->setSingleShot(true);
    m_capabilitiesTimer->setInterval(300);
    connect(m_capabilitiesTimer, &QTimer::timeout, this, [this] {
        m_statusOverlay->showMessage(tr("LOAD_SCANNER_CAPABILITIES..."));
        m_statusOverlay->raise();
    });
    m_settingsOverlay = new SettingsOverlay(nullptr);
    m_settingsOverlay->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

    // A wider or narrower window changes the preview, the page list keeps its width
    // (it changes only by dragging the splitter)
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);

    restoreWindowLayout();
    applySurfaceColors();
}

// Size, position and splitter of the last session (saved in closeEvent). First start: 900 × 600
// (fits laptops with 150 % scaling), page list 180 px. Qt widens the window by itself if the
// toolbar buttons need more room (minimum size of the layout).
void MainWin::restoreWindowLayout() {
    constexpr QSize kDefaultSize(900, 600);
    const SettingsManager &settings = SettingsManager::instance();
    if (!restoreGeometry(settings.windowGeometry())) {
        resize(kDefaultSize);
    }
    if (!ui->splitter->restoreState(settings.splitterState())) {
        // The final width is only known once the window is shown
        QTimer::singleShot(0, this, [this]() {
            int leftWidth = 180;
            int rightWidth = ui->splitter->width() - leftWidth;
            ui->splitter->setSizes({leftWidth, rightWidth});
        });
    }
}

// Page list and preview lie deeper than the window, like the toolbar. Only the viewports get
// the color (Base and Window, depending on what the style uses for the viewport).
// Application palette for the same reason as in ToolbarM (macOS keeps stale widget palettes).
// Called again in changeEvent, so light/dark switches apply live.
void MainWin::applySurfaceColors() {
    const QPalette appPal = QApplication::palette();
    const QColor surface = UiColors::mix(appPal.color(QPalette::Window),
                                         appPal.color(QPalette::WindowText),
                                         UiColors::surfaceShadeFor(appPal));

    const QList<QWidget*> viewports = {
        ui->pagesListWidget->viewport(),
        ui->previewGraphicsView->viewport()
    };
    for (QWidget *viewport : viewports) {
        QPalette pal = viewport->palette();
        pal.setColor(QPalette::Base, surface);
        pal.setColor(QPalette::Window, surface);
        viewport->setPalette(pal);
    }
}

// ====== Connects ======

void MainWin::initializeConnects() {
    // The scan button cancels while scanning, Esc too (and the text recognition while saving)
    connect(m_scanBtn, &QToolButton::clicked, this, [this] {
        if (m_scanProcess) {
            cancelScan();
        } else {
            startScan();
        }
    });
    auto *cancelShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(cancelShortcut, &QShortcut::activated, this, [this] {
        if (m_ocrProcess) {
            cancelOcr();   // text recognition while saving
        } else {
            cancelScan();
        }
    });
    connect(m_settingsBtn, &QToolButton::clicked, this, &MainWin::showSettingsOverlay);
    connect(m_refreshBtn, &QToolButton::clicked, this, &MainWin::refreshScanners);
    connect(ui->pagesListWidget, &QListWidget::currentItemChanged, this, &MainWin::showCurrentPage);
    // Page buttons follow the pages and the selection; a rotated page is shown again
    connect(ui->pagesListWidget, &PageListWidget::pagesChanged, this, &MainWin::updateButtons);
    // Anything changed: the pages are not (all) in a PDF any more; an empty list has nothing to lose
    connect(ui->pagesListWidget, &PageListWidget::pagesChanged, this, [this] {
        m_unsaved = ui->pagesListWidget->count() > 0;
    });
    connect(ui->pagesListWidget, &PageListWidget::pageImagesChanged, this, &MainWin::showCurrentPage);
    connect(ui->pagesListWidget, &QListWidget::itemSelectionChanged, this, &MainWin::updateButtons);
    connect(m_rotateBtn, &QToolButton::clicked, this, &MainWin::rotatePages);
    connect(m_clearBtn, &QToolButton::clicked, this, &MainWin::clearPages);
    connect(m_saveSelBtn, &QToolButton::clicked, this, [this] { savePdf(true); });
    connect(m_saveAllBtn, &QToolButton::clicked, this, [this] { savePdf(false); });
    connect(m_settingsOverlay, &SettingsOverlay::scannerChanged,
            this, &MainWin::getScannerCapabilities);
    connect(m_coffeeBtn, &QToolButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://paypal.me/michaelspeckenbach")));
    });
    connect(m_helpBtn, &QToolButton::clicked, this, [this] { showPage(ui->helpPage); });

    // A newer version out? Then the update button shows up and opens its release page
    m_updateChecker = new UpdateChecker(this);
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this, [this](const QString &version) {
        //: Tooltip of the update button; %1 is the new version, e.g. 0.8.0
        m_updateBtn->setToolTip(tr("Version %1 is available – open its download page").arg(version));
        m_updateBtn->setProperty("version", version);
        m_updateBtn->show();
    });
    connect(m_updateBtn, &QToolButton::clicked, this, [this] {
        QDesktopServices::openUrl(UpdateChecker::releaseUrl(m_updateBtn->property("version").toString()));
    });
    connect(m_settingsOverlay, &SettingsOverlay::checkUpdatesChanged, this, [this](bool enabled) {
        if (enabled) {
            m_updateChecker->check();
        } else {
            m_updateBtn->hide();
        }
    });
    m_updateChecker->check();
    connect(ui->helpPage, &HelpPage::backRequested, this, [this] { showPage(ui->mainPage); });
}

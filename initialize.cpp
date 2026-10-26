#include "mainwin.h"
#include "./ui_mainwin.h"
#include "toolbarm.h"
#include "uicolors.h"

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

    ui->toolBarFrame->addButton("rotateBtn","",":icons/rotate");
    m_rotateBtn = ui->toolBarFrame->findChild<QToolButton*>("rotateBtn");
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
}

// ====== Form ======

void MainWin::initializeForm() {
    // Page list (PageListWidget) and preview (PreviewView) set themselves up

    // Overlays
    m_statusOverlay = new StatusOverlay(this);
    m_settingsOverlay = new SettingsOverlay(nullptr);
    m_settingsOverlay->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

    // Splitter: the final width is only known once the window is shown
    QTimer::singleShot(0, this, [this]() {
        int leftWidth = 180;
        int rightWidth = ui->splitter->width() - leftWidth;
        ui->splitter->setSizes({leftWidth, rightWidth});
    });

    applySurfaceColors();
}

// Page list and preview lie deeper than the window, like the toolbar. Only the viewports get
// the color (Base and Window, depending on what the style uses for the viewport).
// Application palette for the same reason as in ToolbarM (macOS keeps stale widget palettes).
// Called again in changeEvent, so light/dark switches apply live.
void MainWin::applySurfaceColors() {
    const QPalette appPal = QApplication::palette();
    const QColor surface = UiColors::mix(appPal.color(QPalette::Window),
                                         appPal.color(QPalette::WindowText),
                                         UiColors::surfaceShade);

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
    connect(m_scanBtn, &QToolButton::clicked, this, &MainWin::startScan);
    connect(m_settingsBtn, &QToolButton::clicked, this, &MainWin::showSettingsOverlay);
    connect(m_refreshBtn, &QToolButton::clicked, this, &MainWin::refreshScanners);
    connect(ui->pagesListWidget, &QListView::clicked, this, &::MainWin::onFilelistTableViewClicked);
    connect(m_settingsOverlay, &SettingsOverlay::scannerChanged,
            this, &MainWin::getScannerCapabilities);
}

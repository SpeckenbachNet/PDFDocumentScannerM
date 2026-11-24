#include "pdfsavedialog.h"
#include "ocr.h"
#include "settingsmanager.h"
#include "uicolors.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QTreeView>
#include <QPushButton>
#include <QSplitter>

PdfSaveDialog::PdfSaveDialog(QWidget *parent, const QString &filePath)
    : QFileDialog(parent, tr("Save PDF"))
{
    setOption(QFileDialog::DontUseNativeDialog);
    setAcceptMode(QFileDialog::AcceptSave);
    setFileMode(QFileDialog::AnyFile);
    setNameFilter(tr("PDF files (*.pdf)"));
    setDefaultSuffix("pdf");

    // Size and view (list / details, columns, sidebar) of the last save; the state also holds
    // a folder, so the folder is set afterwards
    const SettingsManager &settings = SettingsManager::instance();
    if (!restoreGeometry(settings.saveDialogGeometry())) resize(900, 600);
    const bool stateRestored = restoreState(settings.saveDialogState());
    setDirectory(QFileInfo(filePath).absolutePath());
    selectFile(QFileInfo(filePath).fileName());

    // OCR language, preset from the settings; a change here is for this save only
    m_languageCombo = new QComboBox(this);
    m_hasLanguages = Ocr::fillLanguageCombo(m_languageCombo, settings.ocrLanguage());
    m_languageCombo->setEnabled(m_hasLanguages);

    // Own buttons in a fixed order: Qt's button box stands vertically on the right and orders
    // the buttons by the desktop's rules
    auto *cancelBtn = new QPushButton(tr("Cancel"), this);
    m_ocrBtn = new QPushButton(tr("Save with OCR"), this);
    m_saveBtn = new QPushButton(tr("Save"), this);
    m_saveBtn->setDefault(true);   // Enter saves without OCR
    m_ocrBtn->setAutoDefault(false);
    cancelBtn->setAutoDefault(false);
    if (m_hasLanguages) {
        m_ocrBtn->setToolTip(tr("Makes the text in the PDF searchable and copyable. Takes a few seconds per page."));
    } else {
        const QString missing = tr("Text recognition needs the program tesseract with at least one language "
                                   "(e.g. the package tesseract-data-deu or tesseract-ocr-deu).");
        m_ocrBtn->setToolTip(missing);
        m_languageCombo->setToolTip(missing);
    }
    connect(cancelBtn, &QPushButton::clicked, this, &QFileDialog::reject);
    connect(m_saveBtn, &QPushButton::clicked, this, [this] {
        m_ocr = false;
        accept();   // QFileDialog::accept checks the name, asks before overwriting, opens folders
    });
    connect(m_ocrBtn, &QPushButton::clicked, this, [this] {
        m_ocr = true;
        accept();
    });

    auto *buttonRow = new QHBoxLayout;
    buttonRow->setSpacing(8);
    buttonRow->addStretch();
    buttonRow->addWidget(cancelBtn);
    buttonRow->addWidget(m_ocrBtn);
    buttonRow->addWidget(m_saveBtn);

    // Qt's button box is hidden; its save button still tells when a file name is entered
    if (auto *box = findChild<QDialogButtonBox*>("buttonBox")) {
        m_qtSaveBtn = box->button(QDialogButtonBox::Save);
        if (!m_qtSaveBtn) m_qtSaveBtn = box->button(QDialogButtonBox::Open);
        if (m_qtSaveBtn) m_qtSaveBtn->installEventFilter(this);
        box->hide();
    }

    // Only PDF is offered: the file type row says nothing
    for (const char *name : {"fileTypeCombo", "fileTypeLabel"}) {
        if (auto *widget = findChild<QWidget*>(name)) widget->hide();
    }

    // Below the file name row: language row, then the buttons over the full width
    if (auto *grid = qobject_cast<QGridLayout*>(layout())) {
        grid->setContentsMargins(20, 20, 20, 20);
        grid->setHorizontalSpacing(12);
        grid->setVerticalSpacing(10);
        const int columns = grid->columnCount();
        // The file name reaches into the column of the hidden button box: all right edges in line
        if (auto *edit = findChild<QLineEdit*>("fileNameEdit")) {
            const int index = grid->indexOf(edit);
            if (index >= 0) {
                int row, column, rowSpan, columnSpan;
                grid->getItemPosition(index, &row, &column, &rowSpan, &columnSpan);
                grid->removeWidget(edit);
                grid->addWidget(edit, row, column, rowSpan, columns - column);
            }
        }
        const int row = grid->rowCount();
        grid->addWidget(new QLabel(tr("OCR language:"), this), row, 0);
        grid->addWidget(m_languageCombo, row, 1, 1, columns - 1);
        grid->addLayout(buttonRow, row + 1, 0, 1, columns);
    } else if (auto *box = qobject_cast<QBoxLayout*>(layout())) {
        box->addWidget(m_languageCombo);
        box->addLayout(buttonRow);
    }
    // Sidebar: Qt starts it very narrow (names cut off); wider at the first start
    if (auto *sidebar = findChild<QListView*>("sidebar")) {
        sidebar->setMinimumWidth(170);
        auto *splitter = findChild<QSplitter*>("splitter");
        if (splitter && !stateRestored) splitter->setSizes({200, width() - 200});
    }

    applyFieldStyles();
    updateButtons();
}

// Qt's parts are found by their object names (not public, but unchanged for years): a part
// that is not found keeps Qt's look
void PdfSaveDialog::applyFieldStyles() {
    if (auto *edit = findChild<QLineEdit*>("fileNameEdit")) UiColors::applyInputStyle(edit);
    if (auto *combo = findChild<QComboBox*>("lookInCombo")) UiColors::applyComboStyle(combo);
    UiColors::applyComboStyle(m_languageCombo);
    if (auto *sidebar = findChild<QListView*>("sidebar")) {
        UiColors::applyBoxStyle(sidebar, false, UiColors::FieldCorners::All,
                                UiColors::listItemRules("QListView"));
    }
    if (auto *list = findChild<QListView*>("listView")) {
        UiColors::applyBoxStyle(list, false, UiColors::FieldCorners::All,
                                UiColors::listItemRules("QListView"));
    }
    if (auto *tree = findChild<QTreeView*>("treeView")) {
        UiColors::applyBoxStyle(tree, false, UiColors::FieldCorners::All,
                                UiColors::listItemRules("QTreeView", false));
    }
}

void PdfSaveDialog::changeEvent(QEvent *event) {
    if (event->type() == QEvent::ApplicationPaletteChange) applyFieldStyles();
    QFileDialog::changeEvent(event);
}

void PdfSaveDialog::done(int result) {
    SettingsManager::instance().setSaveDialogLayout(saveGeometry(), saveState());
    QFileDialog::done(result);
}

QString PdfSaveDialog::ocrLanguage() const {
    return m_languageCombo->currentData().toString();
}

// Follow Qt's hidden save button (disabled without a file name)
bool PdfSaveDialog::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_qtSaveBtn && event->type() == QEvent::EnabledChange) updateButtons();
    return QFileDialog::eventFilter(watched, event);
}

void PdfSaveDialog::updateButtons() {
    const bool nameOk = !m_qtSaveBtn || m_qtSaveBtn->isEnabled();
    m_saveBtn->setEnabled(nameOk);
    m_ocrBtn->setEnabled(nameOk && m_hasLanguages);
}

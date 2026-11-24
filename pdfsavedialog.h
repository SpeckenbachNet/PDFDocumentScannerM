#pragma once

#include <QFileDialog>

class QComboBox;
class QPushButton;

// File dialog for saving a PDF. The buttons decide: "Save" or "Save with OCR" (text recognition),
// the OCR language is chosen next to them (preset from the settings, not stored from here).
// Qt's own dialog: the system dialog (desktop portal) cannot take extra buttons.
class PdfSaveDialog : public QFileDialog
{
    Q_OBJECT

public:
    PdfSaveDialog(QWidget *parent, const QString &filePath);

    bool ocr() const { return m_ocr; }   // closed with "Save with OCR"
    QString ocrLanguage() const;         // tesseract code, empty: none installed

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void done(int result) override;   // remembers size and view

private:
    QComboBox *m_languageCombo = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_ocrBtn = nullptr;
    QPushButton *m_qtSaveBtn = nullptr;   // Qt's hidden button: Qt enables it when a name is entered
    bool m_hasLanguages = false;
    bool m_ocr = false;

    void updateButtons();
    void applyFieldStyles();   // rounded fields and lists; again after a light / dark switch
};

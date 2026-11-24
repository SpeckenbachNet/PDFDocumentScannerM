#pragma once

#include <QString>
#include <QStringList>

class QComboBox;

// Text recognition with tesseract (external program) when a PDF is saved
namespace Ocr {

// Installed tesseract languages (codes like "deu", "eng"), without "osd" (page orientation only).
// Empty: tesseract is missing or has no language.
QStringList languages();

// Name of a language in that language ("Deutsch" for "deu"); variants keep their code
// ("Deutsch (deu_latf)"), unknown codes stay as they are
QString languageName(const QString &code);

// Fills the combo with the installed languages (sorted by name, code as item data) and selects
// the wanted one; if it is not installed the one of the system, else the first.
// Returns false if no language is installed (the combo is empty then).
bool fillLanguageCombo(QComboBox *combo, const QString &wanted);

}

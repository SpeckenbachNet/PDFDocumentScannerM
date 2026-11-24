#include "ocr.h"
#include <QComboBox>
#include <QLocale>
#include <QProcess>

namespace Ocr {

QStringList languages() {
    QProcess process;
    process.start("tesseract", {"--list-langs"});
    if (!process.waitForFinished(5000) || process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0) {
        process.kill();
        return {};
    }
    // First line: "List of available languages in "/usr/share/tessdata/" (2):"
    QStringList lines = QString::fromLocal8Bit(process.readAllStandardOutput()).split('\n');
    QStringList codes;
    for (int i = 1; i < lines.size(); ++i) {
        const QString code = lines[i].trimmed();
        if (code.isEmpty() || code == "osd" || code.contains(' ')) continue;
        codes << code;
    }
    return codes;
}

QString languageName(const QString &code) {
    // "deu_latf", "chi_sim": language before the underscore
    const QString base = code.section('_', 0, 0);
    const QLocale::Language language = QLocale::codeToLanguage(base);
    if (language == QLocale::AnyLanguage || language == QLocale::C) return code;
    // World: "English" instead of "American English" (some still name a country, e.g. Spanish)
    QString name = QLocale(language, QLocale::World).nativeLanguageName();
    if (name.isEmpty()) return code;
    name[0] = name[0].toUpper();   // "français" -> "Français"
    return base == code ? name : QString("%1 (%2)").arg(name, code);
}

bool fillLanguageCombo(QComboBox *combo, const QString &wanted) {
    QList<QPair<QString, QString>> entries;   // name, code
    for (const QString &code : languages()) entries.append({languageName(code), code});
    std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
        return QString::localeAwareCompare(a.first, b.first) < 0;
    });

    combo->clear();
    for (const auto &entry : std::as_const(entries)) combo->addItem(entry.first, entry.second);
    if (combo->count() == 0) return false;

    int index = combo->findData(wanted);
    if (index < 0) {
        const QString system = QLocale::languageToCode(QLocale::system().language(), QLocale::ISO639Part2T);
        index = combo->findData(system);
    }
    combo->setCurrentIndex(qMax(index, 0));
    return true;
}

}

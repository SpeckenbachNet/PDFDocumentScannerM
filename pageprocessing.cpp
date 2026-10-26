#include "pageprocessing.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <algorithm>
#include <array>

namespace PageProcessing {

namespace {

// Mode names differ between backends ("Color", "Colour", "24bit Color", "Gray", "True Gray",
// "Gray[Error Diffusion]"): an exact name first, then one that contains the word but is no
// dithered variant
QString findMode(const QStringList &scannerModes, const QStringList &words) {
    for (const QString &mode : scannerModes) {
        for (const QString &word : words) {
            if (mode.compare(word, Qt::CaseInsensitive) == 0) return mode;
        }
    }
    for (const QString &mode : scannerModes) {
        if (mode.contains("diffusion", Qt::CaseInsensitive) || mode.contains("dither", Qt::CaseInsensitive)) {
            continue;
        }
        for (const QString &word : words) {
            if (mode.contains(word, Qt::CaseInsensitive)) return mode;
        }
    }
    return QString();
}

}

QString scannerModeFor(const QString &colorMode, const QStringList &scannerModes) {
    const QString color = findMode(scannerModes, {"color", "colour"});
    if (colorMode == "Color") return color;
    const QString gray = findMode(scannerModes, {"gray", "grey"});
    return gray.isEmpty() ? color : gray;
}

QString originalPath(const QString &pagePath) {
    const QFileInfo info(pagePath);
    return info.dir().filePath("gray_" + info.fileName());
}

int otsuThreshold(const QImage &gray) {
    std::array<qint64, 256> histogram{};
    for (int y = 0; y < gray.height(); ++y) {
        const uchar *line = gray.constScanLine(y);
        for (int x = 0; x < gray.width(); ++x) ++histogram[line[x]];
    }

    const qint64 total = qint64(gray.width()) * gray.height();
    double sumAll = 0;
    for (int i = 0; i < 256; ++i) sumAll += double(i) * histogram[i];

    // The threshold with the largest variance between the dark and the bright group
    double sumDark = 0;
    qint64 countDark = 0;
    double bestVariance = -1;
    int best = 128;
    for (int t = 0; t < 256; ++t) {
        countDark += histogram[t];
        if (countDark == 0) continue;
        const qint64 countBright = total - countDark;
        if (countBright == 0) break;
        sumDark += double(t) * histogram[t];
        const double meanDark = sumDark / countDark;
        const double meanBright = (sumAll - sumDark) / countBright;
        const double variance = double(countDark) * countBright * (meanDark - meanBright) * (meanDark - meanBright);
        if (variance > bestVariance) {
            bestVariance = variance;
            best = t + 1;   // values up to t are dark
        }
    }
    return best;
}

QImage toBlackWhite(const QImage &gray, int threshold) {
    QImage result(gray.size(), QImage::Format_Mono);
    result.setColorTable({qRgb(0, 0, 0), qRgb(255, 255, 255)});   // bit 0 black, bit 1 white
    result.setDotsPerMeterX(gray.dotsPerMeterX());
    result.setDotsPerMeterY(gray.dotsPerMeterY());
    for (int y = 0; y < gray.height(); ++y) {
        const uchar *in = gray.constScanLine(y);
        uchar *out = result.scanLine(y);
        std::fill(out, out + result.bytesPerLine(), 0);
        for (int x = 0; x < gray.width(); ++x) {
            if (in[x] >= threshold) out[x >> 3] |= 0x80 >> (x & 7);   // Format_Mono: first pixel in the high bit
        }
    }
    return result;
}

bool convertScannedPage(const QString &filePath, const QString &colorMode) {
    if (colorMode != "Gray" && colorMode != "BlackWhite") return true;   // color: as scanned

    const QImage scanned(filePath);
    if (scanned.isNull()) return false;
    const bool scannedGray = scanned.format() == QImage::Format_Grayscale8;
    const QImage gray = scannedGray ? scanned : scanned.convertToFormat(QImage::Format_Grayscale8);

    if (colorMode == "Gray") {
        // Scanned in gray already (the usual case): nothing to write
        return scannedGray || gray.save(filePath, "PNG");
    }

    gray.save(originalPath(filePath), "PNG");   // without it only a new threshold is lost
    return toBlackWhite(gray, otsuThreshold(gray)).save(filePath, "PNG");
}

}

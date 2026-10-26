#pragma once

#include <QImage>
#include <QString>

// Processing of a scanned page right after it arrives (color mode now; straighten and crop
// later). Color modes of the app, stored in Scanner/ColorMode: "Color", "Gray", "BlackWhite".
namespace PageProcessing {

// Scanner value (--mode) to scan with for the app's color mode: "Color" in color, "Gray" and
// "BlackWhite" in gray if the scanner offers it, otherwise in color (converted afterwards).
// Empty: the scanner reports no modes, it scans with its default.
QString scannerModeFor(const QString &colorMode, const QStringList &scannerModes);

// Brings the page file to the color mode: gray or 1 bit black/white (automatic threshold).
// For black/white the gray image is kept as originalPath() for a later new threshold.
bool convertScannedPage(const QString &filePath, const QString &colorMode);

// Gray original of a black/white page: gray_page_001.png next to page_001.png
// (the prefix keeps it out of the scan's "page_*.png" filter)
QString originalPath(const QString &pagePath);

// Otsu: the threshold that separates the two brightness groups (paper, ink) best
int otsuThreshold(const QImage &gray);

// 1 bit image: darker than the threshold black, the rest white
QImage toBlackWhite(const QImage &gray, int threshold);

}

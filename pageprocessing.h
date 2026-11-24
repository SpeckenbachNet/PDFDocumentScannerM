#pragma once

#include <QImage>
#include <QString>

// Processing of a scanned page right after it arrives (landscape turn, straighten, color mode;
// crop later). Color modes of the app, stored in Scanner/ColorMode: "Color", "Gray", "BlackWhite".
namespace PageProcessing {

// Scanner value (--mode) to scan with for the app's color mode: "Color" in color, "Gray" and
// "BlackWhite" in gray if the scanner offers it, otherwise in color (converted afterwards).
// Empty: the scanner reports no modes, it scans with its default.
QString scannerModeFor(const QString &colorMode, const QStringList &scannerModes);

// Brings the page file to the color mode: gray or 1 bit black/white (automatic threshold).
// For black/white the gray image is kept as originalPath() for a later new threshold.
// landscape: the page was scanned upright (as it lies on the glass or in the feeder) and is
// turned 90° counter-clockwise first, so it becomes a landscape page.
// straighten: a page that lies slightly askew is turned straight (before the black/white
// threshold, so the letters stay clean). straightenedBy: the angle it was turned by (0: not
// turned), shown in the page list.
bool convertScannedPage(const QString &filePath, const QString &colorMode, bool landscape = false,
                        bool straighten = false, double *straightenedBy = nullptr);

// Angle in degrees by which the text lines of the page are tilted (positive: they fall to the
// right). 0 if the page needs no turn or shows no lines (blank page, photo). Searched up to ±5°.
double skewAngle(const QImage &image);

// Turned by -angle around the center, same size, format and resolution; the uncovered corners
// become white
QImage straightenedImage(const QImage &image, double angle);

// Turned by a multiple of 90° (90 = clockwise) in the same format: Qt turns a 1 bit image into
// 32 bit while rotating, the black/white page would lose its small size
QImage rotatedImage(const QImage &image, int degrees = 90);

// Gray original of a black/white page: gray_page_001.png next to page_001.png
// (the prefix keeps it out of the scan's "page_*.png" filter)
QString originalPath(const QString &pagePath);

// Otsu: the threshold that separates the two brightness groups (paper, ink) best
int otsuThreshold(const QImage &gray);

// 1 bit image: darker than the threshold black, the rest white
QImage toBlackWhite(const QImage &gray, int threshold);

// Uncompressed 1 bit TIFF with the resolution of the image, for tesseract: it takes a PNG into
// the PDF as it is (Flate), a TIFF it encodes as CCITT G4, which is a third smaller for text
// pages. Written by hand, Qt's TIFF writer is a plugin the AppImage may lack.
bool writeBlackWhiteTiff(const QImage &image, const QString &filePath);

}

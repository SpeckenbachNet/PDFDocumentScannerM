#include "pageprocessing.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QStringList>
#include <QTransform>
#include <QVector>
#include <algorithm>
#include <array>
#include <cmath>

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

// Projection profile: every dark point is counted in the row it falls on when the page is
// looked at under the angle. Under the right angle the text lines fall on few rows and the gaps
// between them on none, so neighboring rows differ strongly: the sum of the squared differences
// is the largest there.
double profileScore(const QVector<QPoint> &points, int rows, double degrees) {
    const double slope = std::tan(degrees * M_PI / 180.0);
    QVector<int> histogram(rows, 0);
    const int offset = rows / 2;
    for (const QPoint &point : points) {
        const int row = int(std::lround(point.y() - point.x() * slope)) + offset;
        if (row >= 0 && row < rows) ++histogram[row];
    }
    double score = 0;
    for (int i = 1; i < rows; ++i) {
        const double difference = histogram[i] - histogram[i - 1];
        score += difference * difference;
    }
    return score;
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

bool convertScannedPage(const QString &filePath, const QString &colorMode, bool landscape, bool straighten,
                        double *straightenedBy) {
    if (straightenedBy) *straightenedBy = 0;
    const bool toGray = colorMode == "Gray" || colorMode == "BlackWhite";
    if (!toGray && !landscape && !straighten) return true;   // color, portrait: as scanned

    QImage scanned(filePath);
    if (scanned.isNull()) return false;
    bool turned = false;
    if (landscape) {
        scanned = rotatedImage(scanned, -90);
        turned = true;
    }
    if (straighten) {
        const double angle = skewAngle(scanned);
        if (angle != 0) {
            scanned = straightenedImage(scanned, angle);
            turned = true;
            if (straightenedBy) *straightenedBy = angle;
        }
    }
    if (!toGray) return !turned || scanned.save(filePath, "PNG");

    const bool scannedGray = scanned.format() == QImage::Format_Grayscale8;
    const QImage gray = scannedGray ? scanned : scanned.convertToFormat(QImage::Format_Grayscale8);

    if (colorMode == "Gray") {
        // Scanned in gray already and not turned (the usual case): nothing to write
        return (scannedGray && !turned) || gray.save(filePath, "PNG");
    }

    gray.save(originalPath(filePath), "PNG");   // without it only a new threshold is lost
    return toBlackWhite(gray, otsuThreshold(gray)).save(filePath, "PNG");
}

QImage rotatedImage(const QImage &image, int degrees) {
    if (image.isNull()) return image;
    QImage rotated = image.transformed(QTransform().rotate(degrees));
    if (rotated.format() != image.format()) {
        rotated = rotated.convertToFormat(image.format(), Qt::ThresholdDither);
    }
    if (degrees % 180 != 0) {
        rotated.setDotsPerMeterX(image.dotsPerMeterY());
        rotated.setDotsPerMeterY(image.dotsPerMeterX());
    }
    return rotated;
}

namespace {
// Darker than this counts as content (print, the shadow of a paper edge); light enough for the
// gray print of a till receipt, dark enough for the paper grain and a white lid
constexpr int kContentThreshold = 160;
constexpr double kContentEdgeMm = 3.0;     // edge strip of the scan that is ignored (lid shadow)
constexpr double kContentMarginMm = 4.0;   // room left around the content
constexpr int kContentMinPixels = 3;       // dark pixels a row / column needs (single specks)
}

QRect contentRect(const QImage &image) {
    if (image.isNull()) return QRect();
    const QRect whole = image.rect();
    const QImage gray = image.convertToFormat(QImage::Format_Grayscale8);

    int dpi = qRound(image.dotsPerMeterX() * 0.0254);
    if (dpi <= 0) dpi = 300;
    const int edge = qRound(kContentEdgeMm * dpi / 25.4);
    const int margin = qRound(kContentMarginMm * dpi / 25.4);
    const QRect inner = whole.adjusted(edge, edge, -edge, -edge);
    if (inner.width() <= 0 || inner.height() <= 0) return whole;

    // Dark pixels per row and per column
    QVector<int> rows(gray.height(), 0);
    QVector<int> columns(gray.width(), 0);
    for (int y = inner.top(); y <= inner.bottom(); ++y) {
        const uchar *line = gray.constScanLine(y);
        for (int x = inner.left(); x <= inner.right(); ++x) {
            if (line[x] < kContentThreshold) {
                ++rows[y];
                ++columns[x];
            }
        }
    }
    auto first = [](const QVector<int> &counts) {
        for (int i = 0; i < counts.size(); ++i) {
            if (counts[i] >= kContentMinPixels) return i;
        }
        return -1;
    };
    auto last = [](const QVector<int> &counts) {
        for (int i = counts.size() - 1; i >= 0; --i) {
            if (counts[i] >= kContentMinPixels) return i;
        }
        return -1;
    };
    const int top = first(rows);
    const int left = first(columns);
    if (top < 0 || left < 0) return whole;   // blank page
    const QRect content(QPoint(left, top), QPoint(last(columns), last(rows)));
    return content.adjusted(-margin, -margin, margin, margin).intersected(whole);
}

QImage croppedImage(const QImage &image, const QRect &rect) {
    const QRect area = rect.intersected(image.rect());
    if (image.isNull() || area.isEmpty()) return QImage();
    QImage cropped = image.copy(area);
    cropped.setDotsPerMeterX(image.dotsPerMeterX());   // the size in mm follows from it (PDF page)
    cropped.setDotsPerMeterY(image.dotsPerMeterY());
    return cropped;
}

double skewAngle(const QImage &image) {
    if (image.isNull()) return 0;

    // About 100 dpi are enough to see the lines and keep the search fast
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    if (gray.width() > 1000) gray = gray.scaledToWidth(1000, Qt::SmoothTransformation);

    // Dark points without the outer 3 %: shadows of the sheet edges and the lid are no text
    const int threshold = otsuThreshold(gray);
    const int marginX = gray.width() * 3 / 100;
    const int marginY = gray.height() * 3 / 100;
    const int centerX = gray.width() / 2;
    const int centerY = gray.height() / 2;
    QVector<QPoint> points;
    for (int y = marginY; y < gray.height() - marginY; ++y) {
        const uchar *line = gray.constScanLine(y);
        for (int x = marginX; x < gray.width() - marginX; ++x) {
            if (line[x] < threshold) points.append(QPoint(x - centerX, y - centerY));
        }
    }

    // Almost blank, or mostly dark (photo): no lines to go by
    const qint64 inner = qint64(gray.width() - 2 * marginX) * (gray.height() - 2 * marginY);
    if (inner <= 0) return 0;
    const double darkShare = double(points.size()) / inner;
    if (darkShare < 0.002 || darkShare > 0.35) return 0;

    // Rows for the whole page under ±5° (the width shifts the rows by up to width · tan 5°)
    const int rows = gray.height() + gray.width() / 5 + 2;

    // Coarse in quarter degrees, then fine around the best one
    double best = 0;
    double bestScore = -1;
    QVector<double> coarseScores;
    for (int step = -20; step <= 20; ++step) {
        const double degrees = step * 0.25;
        const double score = profileScore(points, rows, degrees);
        coarseScores.append(score);
        if (score > bestScore) {
            bestScore = score;
            best = degrees;
        }
    }
    const double coarseBest = best;
    for (int step = -10; step <= 10; ++step) {
        const double degrees = coarseBest + step * 0.025;
        const double score = profileScore(points, rows, degrees);
        if (score > bestScore) {
            bestScore = score;
            best = degrees;
        }
    }

    // Text gives one clear peak; a picture without lines scores about the same under every angle
    std::sort(coarseScores.begin(), coarseScores.end());
    const double median = coarseScores.at(coarseScores.size() / 2);
    if (median <= 0 || bestScore < median * 1.5) return 0;

    // Below 0.2° the page looks straight; turning would only soften it a little
    if (std::abs(best) < 0.2) return 0;
    return best;
}

QImage straightenedImage(const QImage &image, double angle) {
    if (image.isNull() || angle == 0) return image;

    // Painted in 32 bit (QPainter does not paint into every format), then back to the format of
    // the page
    const QImage source = image.convertToFormat(QImage::Format_RGB32);
    QImage result(image.size(), QImage::Format_RGB32);
    result.fill(Qt::white);
    {
        QPainter painter(&result);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QPointF center(image.width() / 2.0, image.height() / 2.0);
        painter.translate(center);
        painter.rotate(-angle);
        painter.translate(-center);
        painter.drawImage(0, 0, source);
    }
    if (result.format() != image.format()) {
        result = result.convertToFormat(image.format(), Qt::ThresholdDither);
    }
    result.setDotsPerMeterX(image.dotsPerMeterX());
    result.setDotsPerMeterY(image.dotsPerMeterY());
    return result;
}

bool writeBlackWhiteTiff(const QImage &image, const QString &filePath) {
    const QImage mono = image.format() == QImage::Format_Mono ? image
                                                              : image.convertToFormat(QImage::Format_Mono);
    if (mono.isNull()) return false;
    const quint32 width = mono.width();
    const quint32 height = mono.height();
    const quint32 rowBytes = (width + 7) / 8;
    // Which bit value is black depends on the color table (ours: 0 black, 1 white)
    const bool zeroIsBlack = mono.colorCount() < 2 || qGray(mono.color(0)) <= qGray(mono.color(1));
    const quint32 dpiX = quint32(std::lround(mono.dotsPerMeterX() * 0.0254));
    const quint32 dpiY = quint32(std::lround(mono.dotsPerMeterY() * 0.0254));
    const bool hasResolution = dpiX > 0 && dpiY > 0;

    // Little endian TIFF: header, one IFD (tags sorted), the two resolutions, the image as one strip
    const quint16 tagCount = hasResolution ? 12 : 9;
    const quint32 ifdSize = 2 + tagCount * 12 + 4;
    const quint32 resolutionOffset = 8 + ifdSize;
    const quint32 imageOffset = resolutionOffset + (hasResolution ? 16 : 0);

    QByteArray data;
    auto put16 = [&data](quint16 v) { data.append(char(v & 0xff)).append(char(v >> 8)); };
    auto put32 = [&data](quint32 v) { for (int i = 0; i < 4; ++i) data.append(char((v >> (8 * i)) & 0xff)); };
    auto tag = [&](quint16 id, quint16 type, quint32 value) {
        put16(id);
        put16(type);
        put32(1);
        if (type == 3) {   // SHORT: in the first two bytes of the value field
            put16(quint16(value));
            put16(0);
        } else {
            put32(value);
        }
    };
    data.append("II", 2);
    put16(42);
    put32(8);
    put16(tagCount);
    tag(256, 4, width);                    // ImageWidth
    tag(257, 4, height);                   // ImageLength
    tag(258, 3, 1);                        // BitsPerSample
    tag(259, 3, 1);                        // Compression: none
    tag(262, 3, zeroIsBlack ? 1 : 0);      // Photometric: BlackIsZero / WhiteIsZero
    tag(273, 4, imageOffset);              // StripOffsets
    tag(277, 3, 1);                        // SamplesPerPixel
    tag(278, 4, height);                   // RowsPerStrip
    tag(279, 4, rowBytes * height);        // StripByteCounts
    if (hasResolution) {                   // without it tesseract assumes a resolution itself
        tag(282, 5, resolutionOffset);     // XResolution
        tag(283, 5, resolutionOffset + 8); // YResolution
        tag(296, 3, 2);                    // ResolutionUnit: inch
    }
    put32(0);   // no further IFD
    if (hasResolution) {
        put32(dpiX);
        put32(1);
        put32(dpiY);
        put32(1);
    }
    data.reserve(data.size() + rowBytes * height);
    for (quint32 y = 0; y < height; ++y) {
        data.append(reinterpret_cast<const char *>(mono.constScanLine(int(y))), rowBytes);
    }

    QFile file(filePath);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

}

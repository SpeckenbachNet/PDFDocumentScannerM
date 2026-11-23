#ifndef UICOLORS_H
#define UICOLORS_H

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyle>
#include <QtGlobal>
#include <QtMath>

// Colors for surfaces and frames, derived from Window/WindowText only.
// Base/Light/Mid/Dark are not usable: with the gtk3 platform theme they are (nearly) equal
// to Window, so surfaces using them look flat on GNOME/Niri.
namespace UiColors {

// Surfaces lying deeper than the window: toolbar, page list, preview
constexpr qreal surfaceShade = -0.06;      // dark mode: away from the (bright) text, darker than the window
constexpr qreal surfaceShadeLight = 0.15;  // light mode: towards the (dark) text, a gray like PDF viewers

// Light mode: the window is brighter than its text
inline bool isLight(const QPalette &pal)
{
    return pal.color(QPalette::Window).lightnessF() > pal.color(QPalette::WindowText).lightnessF();
}

inline qreal surfaceShadeFor(const QPalette &pal)
{
    return isLight(pal) ? surfaceShadeLight : surfaceShade;
}

// Frame of the settings overlay
constexpr qreal overlayFrameShade = 0.15;

// Corner radius of selected entries in lists (help topics)
constexpr int entryRadius = 6;

// WCAG contrast ratio of two colors (1 = none, 21 = black on white)
inline qreal contrastRatio(const QColor &a, const QColor &b)
{
    auto luminance = [](const QColor &c) {
        auto lin = [](qreal v) { return v <= 0.03928 ? v / 12.92 : qPow((v + 0.055) / 1.055, 2.4); };
        return 0.2126 * lin(c.redF()) + 0.7152 * lin(c.greenF()) + 0.0722 * lin(c.blueF());
    };
    const qreal la = luminance(a);
    const qreal lb = luminance(b);
    return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
}

// Mixes a towards b by t (t < 0 moves away from b, the result is kept inside 0..1)
inline QColor mix(const QColor &a, const QColor &b, qreal t)
{
    t = qBound<qreal>(-1.0, t, 1.0);
    auto channel = [t](qreal x, qreal y) { return qBound<qreal>(0.0, x * (1.0 - t) + y * t, 1.0); };
    return QColor::fromRgbF(channel(a.redF(),   b.redF()),
                            channel(a.greenF(), b.greenF()),
                            channel(a.blueF(),  b.blueF()),
                            a.alphaF());
}

// The palette of the app: always Fusion's own light or dark palette (style()->standardPalette()
// follows the color scheme). A platform theme (e.g. gtk3) has its own colors for one mode only;
// mixed with Fusion's for the other mode, dark looked different after switching back and forth.
// If the selection color and its text have too little contrast (e.g. accent color "Graphite"
// in dark mode: white on white), the accent color is used for the selection and black or
// white for its text.
// Call after creating the QApplication and again whenever light / dark mode switches.
inline void applyStandardPalette()
{
    constexpr qreal minContrast = 3.0;
    QPalette pal = QApplication::style()->standardPalette();

    for (QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        QColor highlight = pal.color(group, QPalette::Highlight);
        QColor text      = pal.color(group, QPalette::HighlightedText);
        if (contrastRatio(highlight, text) >= minContrast)
            continue;
        highlight = pal.color(group, QPalette::Accent);
        if (contrastRatio(highlight, text) < minContrast)
            text = contrastRatio(highlight, Qt::white) >= contrastRatio(highlight, Qt::black)
                       ? QColor(Qt::white) : QColor(Qt::black);
        pal.setColor(group, QPalette::Highlight, highlight);
        pal.setColor(group, QPalette::HighlightedText, text);
    }
    QApplication::setPalette(pal);
}

}

#endif // UICOLORS_H

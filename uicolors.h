#ifndef UICOLORS_H
#define UICOLORS_H

#include <QColor>
#include <QtGlobal>

// Colors for surfaces and frames, derived from Window/WindowText only.
// Base/Light/Mid/Dark are not usable: with the gtk3 platform theme they are (nearly) equal
// to Window, so surfaces using them look flat on GNOME/Niri.
namespace UiColors {

// Surfaces lying deeper than the window: toolbar, page list, preview
constexpr qreal surfaceShade = -0.06;

// Frame of the settings overlay
constexpr qreal overlayFrameShade = 0.15;

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

}

#endif // UICOLORS_H

#ifndef UICOLORS_H
#define UICOLORS_H

#include <QAbstractScrollArea>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QLineEdit>
#include <QPalette>
#include <QStyledItemDelegate>
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


// ===== Rounded fields (as in NiriSettingsM / MovieMetaM themecolors.h) =====
// The style sheets contain the colors as fixed values: when light / dark mode switches they
// have to be set again.

// Which corners of a field are rounded: an input field on top of a list forms one box with it
enum class FieldCorners { All, Top, Bottom };

// Colors and shape shared by input fields, combo boxes and boxes.
// onBase: content colors (Base / Text) instead of the button colors, e.g. for lists.
struct FieldLook {
    static constexpr int radius = 6;
    static constexpr int height = 26;

    QColor fill;
    QColor text;
    QColor placeholder;
    QColor border;
    QColor focus;
    // Disabled: everything steps back, the fill stays
    QColor disabledText;
    QColor disabledPlaceholder;
    QColor disabledBorder;

    explicit FieldLook(bool onBase = false)
    {
        const QPalette pal = QApplication::palette();
        fill        = pal.color(onBase ? QPalette::Base : QPalette::Button);
        text        = pal.color(onBase ? QPalette::Text : QPalette::ButtonText);
        placeholder = mix(fill, text, 0.50);
        border      = mix(fill, text, 0.30);
        focus       = pal.color(QPalette::Highlight);
        disabledText        = mix(fill, text, 0.35);
        disabledPlaceholder = mix(fill, text, 0.25);
        disabledBorder      = mix(fill, text, 0.15);
    }

    // Style sheet rules for the corners (Top: no bottom radius, Bottom: no top border)
    static QString cornerRules(FieldCorners corners)
    {
        switch (corners) {
        case FieldCorners::Top:
            return QStringLiteral(" border-top-left-radius: %1px; border-top-right-radius: %1px;"
                                  " border-bottom-left-radius: 0; border-bottom-right-radius: 0;").arg(radius);
        case FieldCorners::Bottom:
            return QStringLiteral(" border-top: none; border-top-left-radius: 0; border-top-right-radius: 0;"
                                  " border-bottom-left-radius: %1px; border-bottom-right-radius: %1px;").arg(radius);
        default:
            return QStringLiteral(" border-radius: %1px;").arg(radius);
        }
    }
};

// Rounded input field
inline void applyInputStyle(QLineEdit *edit, FieldCorners corners = FieldCorners::All)
{
    const FieldLook look;

    edit->setFixedHeight(FieldLook::height);
    edit->setStyleSheet(QStringLiteral(
        "QLineEdit { background: %1; color: %2; placeholder-text-color: %3; border: 1px solid %4;"
        "%5 padding: 0 6px; selection-background-color: %6; }"
        "QLineEdit:focus { border: 1px solid %6; }"
        "QLineEdit:disabled { color: %7; placeholder-text-color: %8; border-color: %9; }")
        .arg(look.fill.name(), look.text.name(), look.placeholder.name(), look.border.name(),
             FieldLook::cornerRules(corners), look.focus.name(), look.disabledText.name(),
             look.disabledPlaceholder.name(), look.disabledBorder.name()));
}

// Rounded combo box like the input fields. The arrow is a grey drawing (":icons/chevron_down"):
// visible in light and dark mode without tinting. The open list: entries selected in the accent
// color (its outer corners stay square: it is a window of its own).
inline void applyComboStyle(QComboBox *combo)
{
    const FieldLook look;
    const QColor hover = mix(look.fill, look.text, 0.09);

    combo->setFixedHeight(FieldLook::height);
    // Plain delegate for the open list: the default (menu like) delegate is drawn by the
    // style itself and ignores the item rules of the style sheet (only once: also called
    // again when light / dark mode switches)
    if (!combo->property("plainDelegate").toBool()) {
        combo->setItemDelegate(new QStyledItemDelegate(combo));
        combo->setProperty("plainDelegate", true);
    }
    combo->setStyleSheet(QStringLiteral(
        "QComboBox { background: %1; color: %2; border: 1px solid %3;%4 padding: 0 24px 0 6px; }"
        "QComboBox:hover { background: %6; }"
        "QComboBox:focus { border: 1px solid %5; }"
        "QComboBox:disabled { color: %8; border-color: %9; }"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QComboBox::down-arrow { image: url(:/icons/chevron_down); width: 14px; height: 14px; }"
        "QComboBox QAbstractItemView { background: %1; color: %2; border: 1px solid %3; padding: 2px; outline: 0; }"
        "QComboBox QAbstractItemView::item { min-height: 24px; padding: 0 6px; border-radius: %7px; }"
        "QComboBox QAbstractItemView::item:selected, QComboBox QAbstractItemView::item:hover"
        " { background: palette(highlight); color: palette(highlighted-text); border: none; }")
        .arg(look.fill.name(), look.text.name(), look.border.name(),
             FieldLook::cornerRules(FieldCorners::All), look.focus.name(), hover.name())
        .arg(FieldLook::radius)
        .arg(look.disabledText.name(), look.disabledBorder.name()));
}

// Rounded box (list, text area) in the content colors (Base / Text), border and disabled
// colors mixed from them like in the input fields.
// showFocus: border in the accent color while focused (only where the user types).
// extraStyle: further rules of the widget, e.g. for the list items.
// A widget with an object name gets the rules only for itself: the header of a table is a
// scroll area too and would otherwise get a frame of its own (unlike NiriSettingsM's version).
inline void applyBoxStyle(QAbstractScrollArea *area, bool showFocus,
                          FieldCorners corners = FieldCorners::All,
                          const QString &extraStyle = QString())
{
    const FieldLook look(true);
    QString box = QStringLiteral("QAbstractScrollArea");
    if (!area->objectName().isEmpty()) box += '#' + area->objectName();

    QString style = QStringLiteral(
        "%6 { background: %1; color: %2; border: 1px solid %3;"
        "%4 padding: 2px; selection-background-color: %5; }")
        .arg(look.fill.name(), look.text.name(), look.border.name(),
             FieldLook::cornerRules(corners), look.focus.name(), box);
    if (showFocus)
        style += QStringLiteral("%2:focus { border: 1px solid %1; }").arg(look.focus.name(), box);
    style += QStringLiteral("%3:disabled { color: %1; border-color: %2; }")
                 .arg(look.disabledText.name(), look.disabledBorder.name(), box);

    area->setStyleSheet(style + extraStyle);
}

// List entries in a box: higher rows, selection in the accent color. rounded: corners of the
// selection rounded (not for tables: there every cell would get its own corners).
inline QString listItemRules(const QString &view, bool rounded = true)
{
    return QStringLiteral(
        "%1 { outline: 0; }"
        "%1::item { min-height: 26px; padding: 0 4px; border-radius: %2px; }"
        "%1::item:selected { background: palette(highlight); color: palette(highlighted-text); }")
        .arg(view).arg(rounded ? FieldLook::radius : 0);
}

}

#endif // UICOLORS_H

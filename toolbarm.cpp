/*
 * Copyright (C) 2026 Michael Speckenbach
 * This file is part of PDFDocumentScanner.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "toolbarm.h"

ToolbarM::ToolbarM(QWidget *parent)
    : QFrame(parent)
{
    mainLayout = qobject_cast<QHBoxLayout*>(layout());

    if (!mainLayout) {
        mainLayout = new QHBoxLayout(this);
    }

    mainLayout->setContentsMargins(10, 7, 10, 7);
    mainLayout->setSpacing(8);
    mainLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void ToolbarM::addSpacer()
{
    mainLayout->addStretch(1);
}

QToolButton* ToolbarM::addButton(const QString &objName, const QString &text, const QString &iconPath, bool useSeparator, bool iconRight)
{
    //getSystemColors();

    QToolButton *btn = new QToolButton(this);
    btn->setObjectName(objName);
    btn->setAttribute(Qt::WA_NoSystemBackground); //Prevents the button drawing standard
    btn->setIconSize(QSize(20, 20));
    btn->setFixedHeight(buttonHeight);

    if (!text.isEmpty()) {
        btn->setText(text);
    }

    // --- NEU: Größenlogik für reine Icon-Buttons ---
    if (!iconPath.isEmpty() && text.isEmpty()) {
        // --- CASE: Only Icon
        btn->setFixedSize(buttonHeight * 1.8, buttonHeight);
    } else {
        // Adjust button size to text size.
        int textW = btn->fontMetrics().horizontalAdvance(text);
        if(useSeparator) {
            btn->setFixedWidth(iconSize.width() + 10 + textW + 30); // Icon(22) + Spacing(10) + Text + Padding+Sperator(20+10)
        } else {
            btn->setFixedWidth(iconSize.width() + 10 + textW + 20); // Icon(22) + Spacing(10) + Text + Padding(20)
        }
    }

    // Set non-standard variables as properties
    btn->setProperty("useSeparator", useSeparator);
    btn->setProperty("iconPath", iconPath);
    btn->setProperty("iconRight", iconRight);

    // Bind to eventFilter and add to layout
    btn->installEventFilter(this);
    mainLayout->addWidget(btn);

    return btn;
}

bool ToolbarM::eventFilter(QObject *obj, QEvent *event)
{
    getSystemColors();

    QToolButton *btn = qobject_cast<QToolButton*>(obj);
    if (!btn) return QFrame::eventFilter(obj, event);

    if (event->type() == QEvent::Paint) {

        QPainter p(btn);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform); // Wichtig für Pixmaps!

        QString iconPath = btn->property("iconPath").toString();
        QIcon icon(iconPath);

        bool hasText = !btn->text().isEmpty();
        bool hasIcon = !icon.isNull();
        bool useSeparator = btn->property("useSeparator").toBool();
        bool isHovered = btn->underMouse();
        bool isPressed = btn->isDown();
        bool isEnabled = btn->isEnabled();

        // 1. Hintergrundfarbe
        QColor currentBtnColor = buttonColor;
        QColor currentTxtColor = buttonTextColor;
        QColor currentBorderColor = buttonColor;

        // 1. Berechne die Differenz der Helligkeit (0-255)
        int diff = qAbs(currentBtnColor.lightness() - baseColor.lightness());

        if (!isEnabled) {
            currentBtnColor = disabledColor; // Eine feste Farbe für "Aus"
            currentTxtColor = disabledTextColor;
        } else if (isPressed) {
            currentBtnColor = pressColor;
        } else if (isHovered) {
            currentBtnColor = hoverColor;
        }

        if (diff < 15) {
            if (baseColor.lightness() > 128) {
                // Helles Theme: Rahmen deutlich abdunkeln
                currentBorderColor = currentBtnColor.darker(120);
            } else {
                // Dunkles Theme: Rahmen leicht aufhellen (für subtile Kante)
                currentBorderColor = currentBtnColor.lighter(115);
            }
        }

        // 2. Den Button-Körper zeichnen
        p.setPen(QPen(currentBorderColor, 1));
        p.setBrush(currentBtnColor);

        QRectF r = QRectF(btn->rect()).adjusted(0.5, 0.5, -0.5, -0.5);

        int cornerRadius = 15;
        p.drawRoundedRect(r, cornerRadius, cornerRadius);

        //p.setPen(textColor);
        p.setFont(btn->font());

        if (hasIcon) {
            // get icon in system color
            QPixmap iconPixmap = getTintedPixmap(icon, iconSize, currentTxtColor);
            QRect iconRect(0, 0, iconPixmap.width(), iconPixmap.height());
            int iconW = iconSize.width();
            int iconH = iconSize.height();

            if (hasIcon && hasText) {
                // --- CASE: TEXT AND ICON ---
                // 1. Wir nutzen strikt die vorgegebene iconSize
                // 1. Maße strikt aus der Variablen iconSize nehme
                int textW = p.fontMetrics().horizontalAdvance(btn->text());
                int spacing = 5;

                //bool useSep = btn->property("useSeparator").toBool();
                int sepSpace = useSeparator ? 14 : 0;

                int totalW = iconW + spacing + textW + sepSpace;

                int startX = (btn->width() - totalW) / 2;
                int iconY = (btn->height() - iconH) / 2;

                bool iconRight = btn->property("iconRight").toBool();

                int iconX, textX, sepX;

                if (iconRight) {
                    textX = startX;
                    sepX  = startX + textW + (sepSpace / 2);
                    iconX = startX + textW + sepSpace + spacing;
                } else {
                    iconX = startX;
                    sepX  = startX + iconW + (sepSpace / 2);
                    textX = startX + iconW + sepSpace + spacing ;
                }

                // --- DRAW ---
                // Icon
                QRect targetIconRect(iconX, iconY, iconW, iconH);
                p.drawPixmap(targetIconRect, iconPixmap);

                // Text
                QRect textRect(textX, 0, textW, btn->height());
                p.setPen(currentTxtColor);
                p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, btn->text());

                // Separator
                if (useSeparator) {
                    p.setPen(QPen(lightColor, 1));
                    p.drawLine(sepX, 7, sepX, btn->height() - 7);
                }
            } else {

                // --- CASE ONLY ICON ---
                // Wir berechnen die Mitte basierend auf der logischen iconSize (22px)
                int iconX = (btn->width() - iconW) / 2;
                int iconY = (btn->height() - iconH) / 2;

                QRect targetIconRect(iconX, iconY, iconW, iconH);
                p.drawPixmap(targetIconRect, iconPixmap);
            }
        } else if (hasText) {
            // --- CASE: ONLY TEXT ---
            p.setPen(currentTxtColor);
            p.drawText(btn->rect(), Qt::AlignCenter, btn->text());
        }
        return true;
    }
    return QFrame::eventFilter(obj, event);
}

QPixmap ToolbarM::getTintedPixmap(const QIcon &icon, const QSize &size, const QColor &color)
{
    if (icon.isNull()) return QPixmap();

    // WICHTIG: Berücksichtige den Skalierungsfaktor des Bildschirms (z.B. 2.0 für 4K)
    qreal dpr = this->devicePixelRatioF();

    // Wir laden das Bild in der physisch benötigten Auflösung (z.B. 44x44 statt 22x22)
    QPixmap pixmap = icon.pixmap(size * dpr);

    // Wir sagen dem Pixmap, dass es sich wie ein 'size' großes Bild verhalten soll
    pixmap.setDevicePixelRatio(dpr);

    QPainter pixPainter(&pixmap);
    pixPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    pixPainter.fillRect(pixmap.rect(), color);
    pixPainter.end();

    return pixmap;
}

void ToolbarM::getSystemColors() {

    baseColor         = palette().color(QPalette::Base);
    buttonColor       = palette().color(QPalette::Button);
    buttonTextColor   = palette().color(QPalette::ButtonText);
    darkColor         = palette().color(QPalette::Dark);
    disabledColor     = palette().color(QPalette::Disabled, QPalette::Button);
    disabledTextColor = palette().color(QPalette::Disabled, QPalette::ButtonText);
    hoverColor        = palette().color(QPalette::Light);
    lightColor        = palette().color(QPalette::Light);
    midColor          = palette().color(QPalette::Mid);
    midLightColor     = palette().color(QPalette::Midlight);
    pressColor        = palette().color(QPalette::Dark);
    shadowColor       = palette().color(QPalette::Shadow);
    textColor         = palette().color(QPalette::Text);
    windowColor       = palette().color(QPalette::Window);

}


void ToolbarM::changeEvent(QEvent *event) {
    // qDebug() << "ENTER changeEvent";
    // if (event->type() == QEvent::ApplicationPaletteChange) {
    //     qDebug() << "Pallete changed";
    //     updateThemeColors();
    // }
    // QFrame::changeEvent(event);
}

void ToolbarM::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = 18.0;
    const qreal diameter = radius * 2.0;

    getSystemColors();

    // 1. Hintergrund
    QPainterPath bg;
    bg.addRoundedRect(r, radius, radius);
    p.fillPath(bg, baseColor);

    // --- Dunkle Kante (Oben + Links + 10% Ecke oben-rechts) ---
    QPainterPath darkPath;
    // Startet in der Kurve oben-rechts bei ca. 45 Grad (oder 10% = 9 Grad)
    // Winkel bei Qt: 0 = rechts, 90 = oben, 180 = links, 270 = unten
    darkPath.arcMoveTo(r.right() - diameter, r.top(), diameter, diameter, 45);
    darkPath.arcTo(r.right() - diameter, r.top(), diameter, diameter, 45, 45); // Rest oben-rechts
    darkPath.lineTo(r.left() + radius, r.top());                              // Oben gerade
    darkPath.arcTo(r.left(), r.top(), diameter, diameter, 90, 90);            // Oben-links voll
    darkPath.lineTo(r.left(), r.bottom() - radius);                          // Links gerade
    darkPath.arcTo(r.left(), r.bottom() - diameter, diameter, diameter, 180, 45); // 10% unten-links

    p.setPen(QPen(midColor));
    p.drawPath(darkPath);

    // --- Helle Kante (Unten + Rechts + 10% Ecke unten-links) ---
    QPainterPath lightPath;
    // Startet in der Kurve unten-links (da wo darkPath aufhörte)
    lightPath.arcMoveTo(r.left(), r.bottom() - diameter, diameter, diameter, 225);
    lightPath.arcTo(r.left(), r.bottom() - diameter, diameter, diameter, 225, 45); // Rest unten-links
    lightPath.lineTo(r.right() - radius, r.bottom());                             // Unten gerade
    lightPath.arcTo(r.right() - diameter, r.bottom() - diameter, diameter, diameter, 270, 90); // Unten-rechts voll
    lightPath.lineTo(r.right(), r.top() + radius);                                // Rechts gerade
    lightPath.arcTo(r.right() - diameter, r.top(), diameter, diameter, 0, 45);    // 10% oben-rechts

    p.setPen(QPen(midLightColor));
    p.drawPath(lightPath);
}

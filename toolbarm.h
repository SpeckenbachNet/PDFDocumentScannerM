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
#pragma once

#include <QApplication>
#include <QPalette>
#include <QHBoxLayout>
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <QSize>
#include <QSpacerItem>
#include <QString>
#include <QStyleOption>
#include <QTimer>
#include <QToolButton>

class ToolbarM : public QFrame {
    Q_OBJECT

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

public:
    // Three options to stlye the button
    // enum ButtonType {
    //     NormalText,      // Only text (standard)
    //     IconOnly,        // Only icon
    //     TextAndIcon      // Text and Icon
    // };

    explicit ToolbarM(QWidget *parent = nullptr);

    // One Button Function for all
    QToolButton* addButton(const QString &objName,
                           const QString &text,
                           const QString &iconPath,
                           bool iconRight = false,
                           bool useSeparator = false);
    void addSpacer();

private:
    QHBoxLayout *mainLayout = nullptr;
    QColor  windowColor,
            textColor,
            borderColor,
            darkColor,
            lightColor,
            hoverColor,
            pressColor,
            baseColor,
            shadowColor,
            midColor,
            disabledColor,
            disabledTextColor,
            buttonColor,
            buttonTextColor,
            midLightColor;

    QPalette pal;
    QPixmap getTintedPixmap(const QIcon &icon, const QSize &size, const QColor &color);
    QString getBaseButtonStyle(); // Central CSS for all Buttons
    int buttonHeight = 32;
    QSize iconSize{22, 22};
    int m_isUpdating = false;
    void updateThemeColors();
    void getSystemColors();
};

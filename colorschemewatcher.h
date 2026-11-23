/*
 * Copyright (C) 2026 Michael Speckenbach
 * This file is part of PDFDocumentScannerM.
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

#include <QObject>
#include <QDBusVariant>

// Reads the light / dark setting of the system from the XDG desktop portal
// (org.freedesktop.appearance / color-scheme) and reports changes at once.
// Works the same under niri, KDE and GNOME.
class ColorSchemeWatcher : public QObject
{
    Q_OBJECT

public:
    // Values as in the portal specification
    enum Scheme { NoPreference = 0, Dark = 1, Light = 2 };
    Q_ENUM(Scheme)

    explicit ColorSchemeWatcher(QObject *parent = nullptr);

    Scheme scheme() const { return m_scheme; }

signals:
    void schemeChanged(ColorSchemeWatcher::Scheme scheme);

private slots:
    void onSettingChanged(const QString &nameSpace, const QString &key, const QDBusVariant &value);

private:
    Scheme m_scheme = NoPreference;
};
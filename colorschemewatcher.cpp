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

#include "colorschemewatcher.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>

namespace {
const QString kService   = QStringLiteral("org.freedesktop.portal.Desktop");
const QString kPath      = QStringLiteral("/org/freedesktop/portal/desktop");
const QString kInterface = QStringLiteral("org.freedesktop.portal.Settings");
const QString kNamespace = QStringLiteral("org.freedesktop.appearance");
const QString kKey       = QStringLiteral("color-scheme");
}

ColorSchemeWatcher::ColorSchemeWatcher(QObject *parent)
    : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    // 1. Ask for the current value once
    QDBusMessage msg = QDBusMessage::createMethodCall(kService, kPath, kInterface,
                                                      QStringLiteral("ReadOne"));
    msg << kNamespace << kKey;
    QDBusReply<QDBusVariant> reply = bus.call(msg);
    if (reply.isValid())
        m_scheme = static_cast<Scheme>(reply.value().variant().toUInt());

    // 2. Register for changes (this is how Chromium / Brave switch at once)
    bus.connect(kService, kPath, kInterface, QStringLiteral("SettingChanged"),
                this, SLOT(onSettingChanged(QString,QString,QDBusVariant)));
}

void ColorSchemeWatcher::onSettingChanged(const QString &nameSpace, const QString &key,
                                          const QDBusVariant &value)
{
    if (nameSpace != kNamespace || key != kKey)
        return;

    const auto newScheme = static_cast<Scheme>(value.variant().toUInt());
    if (newScheme == m_scheme)
        return;

    m_scheme = newScheme;
    emit schemeChanged(m_scheme);
}
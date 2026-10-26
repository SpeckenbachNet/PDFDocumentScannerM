# PDFDocumentScannerM – Entwickler-Dokumentation

[English](DEVELOPMENT.md)

Stand: Version 0.3.4 (Oktober 2026). Code, Kommentare, Commit-Nachrichten und die Quelltexte der
Übersetzungen sind Englisch. Diese Doku gibt es auf Englisch und Deutsch; beide zusammen ändern.

---

## 1. Überblick

PDFDocumentScannerM scannt Dokumente mit Netzwerkscannern und speichert sie (am Ende) als PDF
(Qt 6 Widgets, C++17, CMake, Linux). Die App spricht nicht direkt mit SANE, sondern ruft die
Kommandozeilenwerkzeuge `airscan-discover` und `scanimage` auf (`QProcess`).

**Stand**

| Funktion | Stand |
|---|---|
| Scannersuche (`airscan-discover`, Rückfall `scanimage -L`), gespeicherter Scanner wird gemerkt | fertig |
| Scanner-Optionen (Quelle, Farbmodus, Scan-Zweck, Auflösung) aus `scanimage --help` | fertig |
| Scannen Flachbett (eine Seite) und Einzug (Stapel), Seiten erscheinen schon während des Scans | fertig |
| Seitenliste: auswählen, entfernen (×, Entf, Kontextmenü), per Drag and Drop umsortieren | fertig |
| Große Vorschau der aktuellen Seite mit Zoom | fertig |
| Fenstergröße und Splitter werden gemerkt | fertig |
| Hilfeseite | Rahmen fertig, Texte meist noch „Folgt.“ |
| Als PDF speichern (Auswahl / alle Seiten), drehen, leeren, Fake-Duplex | offen (Knöpfe da, nicht verbunden) |
| OCR, Begradigen, Zuschneiden (Häkchen in den Einstellungen) | offen (nicht angebunden) |

**Ablauf**

```
 Start ──► Scannersuche ──► Optionen des gewählten Scanners ──► Scannen
                                                                   │
            scanimage schreibt page_001.png, page_002.png, … nach /tmp/scan_job_<Zeit>/
                                                                   │
                     Seitenliste (neue Seite = aktuelle) ──► Vorschau
```

Das Fenster hat zwei Seiten (`stackedWidget`, jede mit eigener Werkzeugleiste, `showPage()`
wechselt): die Hauptseite (Werkzeugleiste, links Seitenliste, rechts Vorschau, dazwischen der
Splitter) und die Hilfeseite. Die Einstellungen sind ein Popup (`SettingsOverlay`) unter dem
Einstellungsknopf.

---

## 2. Einrichten und Bauen

### Voraussetzungen

Getestet unter CachyOS (Arch). Die Pakete zum Bauen heißen wie unter Arch; die Tabellen zur
Laufzeit nennen auch andere Distributionen.

**Bauen**

| Paket | getestet | wofür |
|---|---|---|
| `cmake` | 4.4.4 | Build (mindestens 3.16) |
| `qt6-base`, `qt6-svg` | 6.12.0 | Widgets, SVG-Icons |
| `qt6-tools` | 6.12.0 | `lupdate` / `lrelease` für die `.ts`-Dateien |

**Laufzeit**

| wofür | getestet (Arch) | Arch/CachyOS | Debian/Ubuntu | Fedora/RHEL | openSUSE |
|---|---|---|---|---|---|
| `scanimage` (Optionen, Scannen) und das `escl`-Backend | 1.4.0 | `sane` | `sane-utils` | `sane-backends` | `sane-backends` |
| `airscan-discover`, die schnelle Scannersuche | 0.99.38 | `sane-airscan` | `sane-airscan` | `sane-airscan` | `sane-airscan` |
| mDNS (Dienst `avahi-daemon` läuft), findet die Netzwerkscanner | 0.9rc5 | `avahi` | `avahi-daemon` | `avahi` | `avahi` |
| OCR – **noch nicht eingebaut**, geplant | 5.5.3 | `tesseract` | `tesseract-ocr` | `tesseract` | `tesseract-ocr` |

Ohne `sane-airscan` fällt die App auf `scanimage -L` zurück (ca. 10 s, verpasst oft Geräte).

**OCR-Sprachdaten:** ein Paket für jede OCR-Sprache, die man wählen kann (Deutsch und
Englisch). Unter Arch ist Englisch **nicht** in `tesseract` enthalten (es zieht nur `osd` und
irgendein Sprachpaket mit).

| Sprache | Arch/CachyOS | Debian/Ubuntu | Fedora/RHEL | openSUSE |
|---|---|---|---|---|
| Deutsch | `tesseract-data-deu` | `tesseract-ocr-deu` | `tesseract-langpack-deu` | `tesseract-ocr-traineddata-german` |
| Englisch | `tesseract-data-eng` | `tesseract-ocr-eng` | `tesseract-langpack-eng` | `tesseract-ocr-traineddata-english` |

Getestet ist nur CachyOS; die Paketnamen der anderen Distributionen sind nicht getestet.

**Nicht nötig:** `hplip`. Sein `hpaio`-Backend scannt den HP M277dw nicht; die App benutzt `escl`.

### Scanner und Netzwerk

Nichts davon ergibt sich aus dem Quelltext:

- Der Scanner muss im Netzwerk **eSCL** (AirScan) anbieten und eine **IPv4-Adresse** haben. Das
  `escl`-Backend von SANE 1.4 kann keine IPv6-URLs öffnen.
- Eine **feste IP** (DHCP-Reservierung im Router) ist empfehlenswert: Die Geräte-ID enthält die
  Adresse. Ändert sie sich, ist der gespeicherte Scanner „weg“ und muss neu gewählt werden.
- `escl` muss in `/etc/sane.d/dll.conf` aktiv sein (Standard).
- **HP Color LaserJet MFP M277dw:** In der Weboberfläche des Druckers muss die Umleitung von
  HTTP auf HTTPS ausgeschaltet sein, sonst scheitert `escl`.
- **Brother MFC-L2710DW:** läuft ohne Anpassungen. `airscan-discover` meldet ihn manchmal nur
  über WSD (etwa 1 von 6 Läufen); dann greift die direkte Abfrage des gespeicherten Scanners
  (Abschnitt 4).

### Bauen

Am einfachsten: `CMakeLists.txt` in **Qt Creator** öffnen, Kit wählen, bauen und starten.

Auf der Kommandozeile:

```
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

Hinweise:

- **Neue Quelldateien zuerst anlegen, dann in `CMakeLists.txt` eintragen.** Qt Creator
  konfiguriert sofort neu; fehlt eine Datei, ist sein Build-Ordner kaputt.
- Defines aus `CMakeLists.txt`: `PROJECT_NAME`, `PROJECT_VERSION`, `COMPANY_NAME`.
- Nach Änderungen an `CMakeLists.txt` CMake neu laufen lassen (Qt Creator macht das meist selbst).

---

## 3. Projektaufbau

| Datei | Inhalt |
|---|---|
| `main.cpp` | Programm- und Organisationsname, INI-Format, Übersetzer, Hauptfenster |
| `mainwin.*`, `initialize.cpp` | Hauptfenster: Scannersuche, Optionen, Scannen, Knopfzustände; `initialize.cpp` baut Werkzeugleisten, Formular, Verbindungen und stellt das Fensterlayout wieder her |
| `settingsmanager.*` | Singleton: alle Einstellungen (INI) und die Optionen des aktuellen Scanners |
| `settingsoverlay.*` | Einstellungs-Popup mit Nase unter dem Einstellungsknopf |
| `statusoverlay.*` | Meldungen über dem Fenster (Suche, Laden, Scannen, Fehler) |
| `pagelistwidget.*` | Seitenliste: eine Spalte, Seitengröße folgt der Breite; auswählen, entfernen, Drag and Drop |
| `pagedelegate.*` | zeichnet eine Seite: Bild, Beschriftung „Seite N“, Auswahl, × beim Überfahren |
| `previewview.*` | große Vorschau (`QGraphicsView`), passt das Bild in die Ansicht ein |
| `helppage.*`, `twolinedelegate.*` | Hilfeseite (Markdown in `help/<de|en>/`, `Help.qrc`); Kopien aus MovieMetaM, an `UiColors` angepasst |
| `uicolors.h` | Flächenfarben, nur aus Window/WindowText abgeleitet |
| `toolbarm.*` | Werkzeugleiste; Kopie aus `../libraries/toolbarm` (Master dort, Version 1.7.0) |
| `globals.h` | leer (Kandidat zum Aufräumen) |

---

## 4. Scanner

### Suche

`MainWin::searchForScanners()` läuft beim Start und mit dem Aktualisieren-Knopf.

- Ist `airscan-discover` installiert (`QStandardPaths::findExecutable`), wird es benutzt: 1–3 s,
  findet beide Testscanner zuverlässig. Sonst `scanimage -L` (ca. 10 s, seine mDNS-Suche verpasst
  oft ein Gerät).
- `saveAirscanScanners()` nimmt nur Zeilen mit `, eSCL` und IPv4-Adresse und macht daraus
  **escl**-Geräte-IDs:
  `Brother MFC-L2710DW series = http://192.168.178.23:80/eSCL/, eSCL` →
  `escl:http://192.168.178.23:80`, angezeigt als `escl: Brother MFC-L2710DW series (192.168.178.23)`.
- Warum nicht die airscan-IDs? `airscan:e0:…`, `airscan:w3:…` enthalten eine laufende Nummer,
  die sich bei jeder Suche ändert; man kann sie nicht speichern. Die escl-ID ist die URL und
  bleibt gleich.
- `saveScanners()` wertet `scanimage -L` aus (Rückfall); Webcams (`v4l`) und IPv6-escl-URLs
  werden übersprungen.

### Auswählen und Merken

- `SettingsManager` hält zwei Scanner: `savedDeviceId()` (die Wahl des Benutzers, INI
  `Scanner/DeviceId` + `Scanner/DeviceName`) und `deviceId()` (in dieser Sitzung benutzt).
- Nur `chooseDevice()` schreibt die INI – aufgerufen, wenn der Benutzer im Overlay einen Scanner
  wählt. `setDeviceId()` ändert nur die Sitzung.
- `chooseScanner()`: Hat die Suche den gespeicherten Scanner verpasst, kommt er trotzdem in die
  Liste (mit dem gespeicherten Namen) und wird **direkt** abgefragt: `escl` öffnet die URL ohne
  Suche (gemessen 0,05–0,3 s). `m_probingSavedScanner` merkt sich das.
- Antwortet er auch so nicht (`getScannerCapabilities` scheitert), entfernt ihn
  `useStandInScanner()` und nimmt für diese Sitzung den ersten gefundenen Scanner. Die INI behält
  die Wahl des Benutzers; die nächste Suche versucht es wieder.

### Optionen

`getScannerCapabilities()` ruft `scanimage -d <id> --help` auf und liest `--resolution`,
`--mode`, `--source`, `--scan-intent` (Werteliste und Standard in `[...]`). Bereiche wie
`50..1200` werden zu den üblichen Auflösungen darin (`expandResolutionValues`). Optionen mit
`[inactive]` werden übersprungen. Ein gespeicherter Wert, den der Scanner nicht anbietet, wird
durch seinen Standard ersetzt.

---

## 5. Scannen und Seiten

### Scannen (`MainWin::startScan()`)

- `scanimage` mit `--device-name` und nur den Optionen, die der Scanner gemeldet hat (sonst
  bricht `scanimage` ab), `-x`/`-y` aus dem Seitenformat (Querformat tauscht sie),
  `--format=png`.
- Einzug (Quelle enthält ADF/Feeder/Duplex, `isAdfSource`): `--batch=page_%03d.png`.
  Flachbett: eine Seite (`--batch-count=1`).
- Zielordner `/tmp/scan_job_<yyyyMMdd_hhmmss>/`. Ein Timer (1,5 s) holt fertige Seiten schon
  während des Scans ab (erst kopieren, damit nie eine halb geschriebene Datei geladen wird).
- Exit-Codes 5 und 7 nach mindestens einer Seite heißen „Einzug leer“, kein Fehler.
- Scheitert der Scan, wird die letzte Zeile von stderr angezeigt.

### Seitenliste (`PageListWidget`, `PageDelegate`)

- ListMode, eine Spalte. Die Seiten füllen die Breite (A4-Verhältnis). Passen sie nicht in die
  Höhe, schrumpfen sie höchstens um die Breite des Rollbalkens, bevor der Balken erscheint
  (`updateItemSizes`).
- Auswählen: Klick, Strg/Umschalt + Klick, Strg+A. Entfernen: × beim Überfahren, Entf/Rücktaste,
  Kontextmenü „n Seiten entfernen“ – **die Bilddatei wird mit gelöscht**. Die Beschriftungen
  werden neu nummeriert.
- Umsortieren: Drag and Drop (`InternalMove`). ListMode, weil IconMode abgelegte Einträge frei
  platziert.
- **Das automatische Rollen beim Ziehen ist unser eigenes:** Das von Qt liest `QCursor::pos()`,
  und das bewegt sich unter Wayland während eines Drags nicht. Ein Timer rollt, solange die
  Drag-Position (aus den Drag-Ereignissen) innerhalb von 40 px am oberen oder unteren Rand liegt
  (`kDragScrollZone`, `kDragScrollMaxStep`, `kDragScrollInterval`).
- Signal `pagesChanged()` nach Hinzufügen, Entfernen und Verschieben. Die Vorschau folgt
  `currentItemChanged` (`MainWin::showCurrentPage`).

### Vorschau (`PreviewView`)

- Eine neue Seite wird ganz gezeigt; in diesem Einpass-Modus folgt die Seite jeder
  Größenänderung. Der Splitter gibt ein breiteres Fenster der Vorschau, die Seitenliste behält
  ihre Breite (Dehnungsfaktoren 0 / 1).
- Zoom: **Strg + Mausrad** (an der Maus), Strg + Plus / Minus / 0, Doppelklick (ganze Seite ↔
  100 %) oder die Zoomleiste unten rechts, die erscheint, solange die Maus über der Vorschau ist
  (−, Stufe – Klick: ganze Seite –, +). Das Mausrad allein rollt, deshalb zoomt es nicht.
- Gezoomt bleibt die Zoomstufe bei Größenänderungen erhalten, Ziehen verschiebt die Seite
  (`ScrollHandDrag`). Bereich: ganze Seite bis 8-fach davon, mindestens 100 %.
- Der Einpass-Maßstab wird aus `maximumViewportSize()` berechnet statt mit `fitInView()`: Das
  misst den Viewport, solange die Rollbalken der gezoomten Seite noch da sind.
- Die Tastenkürzel gelten im ganzen Fenster; solange die Hilfeseite zu sehen ist, lösen sie
  nicht aus (verstecktes Widget).

---

## 6. Einstellungen (QSettings)

`main.cpp` setzt die Organisation `SpeckenbachNet` (`ORGANIZATION_NAME` in `CMakeLists.txt`),
den Programmnamen und das INI-Format. Datei: `~/.config/SpeckenbachNet/PDFDocumentScannerM.ini`.
Aller Zugriff läuft über `SettingsManager`.

| Schlüssel | Inhalt |
|---|---|
| `Scanner/DeviceId`, `Scanner/DeviceName` | der Scanner des Benutzers (nur bei seiner Wahl geschrieben) |
| `Scanner/ScanningUnit`, `ColorMode`, `Resolution`, `ScanIntent` | Scanner-Optionen |
| `Scanner/PageOrientation`, `PageFormat` | Seite |
| `Scanner/OcrLanguage` | tesseract-Code (`deu`, `eng`) |
| `Window/Geometry`, `Window/Splitter` | `saveGeometry()` / `QSplitter::saveState()`, geschrieben in `closeEvent` |

Erster Start ohne gespeichertes Fenster: 900 × 600 (`kDefaultSize` in `restoreWindowLayout()`,
passt auf Laptops mit 150 % Skalierung), Seitenliste 180 px. Braucht die Werkzeugleiste mehr
Platz, macht Qt das Fenster selbst breiter. Unter Wayland wird die Fensterposition nicht
wiederhergestellt (das entscheidet der Compositor).

---

## 7. Übersetzungen

- `i18n/PDFDocumentScannerM_de.ts`, `_en.ts`; `qt_add_translations` baut die `.qm`-Dateien nach
  `i18n/`, `translation.qrc` bettet sie ein.
- Viele Quelltexte sind Schlüssel (`SCAN`, `REMOVE_PAGES %n`), deshalb braucht auch Englisch eine
  Übersetzung. Neuere Texte sind normale englische Sätze.
- `main.cpp` lädt die erste Systemsprache, für die es eine Übersetzung gibt, sonst Englisch.
- Hilfetexte: `help/de/*.md`, `help/en/*.md` (`Help.qrc`).

---

## 8. UI-Bausteine und Stil

- **Farben werden nur aus Window/WindowText abgeleitet** (`uicolors.h`, `UiColors::mix`). Mit
  dem gtk3-Platform-Theme sind Window, Button und Base gleich und Light/Mid/Dark fast gleich;
  daraus genommene Farben verschwinden.
- Seitenliste und Vorschau liegen tiefer als das Fenster, wie die Werkzeugleiste
  (`applySurfaceColors`, bei Palettenwechsel erneut aufgerufen, so wirkt ein Wechsel hell/dunkel
  sofort).
- `ToolbarM`: Knöpfe mit fester Größe aus Text, Icon und Schrift; `addSpacer()` für Gruppen.
  `applyToolbarStyle()` in `initialize.cpp` setzt die Abstufungen für dunkel und hell.
- Knopfzustände an einer Stelle: `setBusy()` / `updateButtons()`. Alle Knöpfe starten gesperrt.

---

## 9. Ressourcen und Lizenzen

- Lizenz: **GPL 3**. Repository: https://github.com/SpeckenbachNet/PDFDocumentScannerM
- Icons: `ressources/icons/*.svg` aus der Icon-Bibliothek in `../libraries/icons` (`icons.qrc`).

---

## 10. Versionen und Commits

- Die Version ist `PROJECT_VERSION` in `CMakeLists.txt`. Jede neue Version ist ein eigener
  Commit; die erste Zeile der Nachricht endet mit `(vX.Y.Z)`.
- Commit-Nachrichten auf Englisch (ältere Commits bis v0.3.2 sind deutsch).

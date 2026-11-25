# PDFDocumentScannerM – Entwickler-Dokumentation

[English](DEVELOPMENT.md)

Stand: Version 0.5.2 (November 2026). Code, Kommentare, Commit-Nachrichten und die Quelltexte der
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
| Scannen Flachbett (eine Seite) und Einzug (Stapel), Seiten erscheinen schon während des Scans, abbrechen (Knopf, Esc) | fertig |
| Seitenliste: auswählen, entfernen (×, Entf, Kontextmenü), per Drag and Drop umsortieren | fertig |
| Große Vorschau der aktuellen Seite mit Zoom | fertig |
| Fenstergröße und Splitter werden gemerkt | fertig |
| Hilfeseite | Rahmen fertig, Texte meist noch „Folgt.“ |
| Als PDF speichern (Auswahl / alle Seiten), drehen, leeren | fertig |
| OCR beim Speichern (tesseract, Knöpfe „Speichern“ / „Mit OCR speichern“ und Sprache im Speicherdialog) | fertig |
| Fake-Duplex | offen (Knopf da, nicht verbunden) |
| Ausrichten gleich nach dem Scan (Häkchen in den Einstellungen) | fertig |
| Beschneiden (Häkchen in den Einstellungen, ausgegraut) | offen |

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
| `qt6-base`, `qt6-svg` | 6.12.0 | Widgets, D-Bus (Hell/Dunkel), SVG-Icons |
| `qt6-tools` | 6.12.0 | `lupdate` / `lrelease` für die `.ts`-Dateien |

**Laufzeit**

| wofür | getestet (Arch) | Arch/CachyOS | Debian/Ubuntu | Fedora/RHEL | openSUSE |
|---|---|---|---|---|---|
| `scanimage` (Optionen, Scannen) und das `escl`-Backend | 1.4.0 | `sane` | `sane-utils` | `sane-backends` | `sane-backends` |
| `airscan-discover`, die schnelle Scannersuche | 0.99.38 | `sane-airscan` | `sane-airscan` | `sane-airscan` | `sane-airscan` |
| mDNS (Dienst `avahi-daemon` läuft), findet die Netzwerkscanner | 0.9rc5 | `avahi` | `avahi-daemon` | `avahi` | `avahi` |
| OCR beim Speichern (optional) | 5.5.4 | `tesseract` | `tesseract-ocr` | `tesseract` | `tesseract-ocr` |

Ohne `sane-airscan` fällt die App auf `scanimage -L` zurück (ca. 10 s, verpasst oft Geräte).

**OCR-Sprachdaten:** ein Paket für jede OCR-Sprache; der Speicherdialog bietet an, was
`tesseract --list-langs` meldet (ohne `osd`). Unter Arch ist Englisch **nicht** in `tesseract`
enthalten (es zieht nur `osd` und irgendein Sprachpaket mit); Debians `tesseract-ocr` bringt
Englisch mit. Ohne tesseract oder ohne Sprache ist „Mit OCR speichern“ ausgegraut.

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
| `main.cpp` | Programm- und Organisationsname, Fusion-Stil, Hell/Dunkel, INI-Format, Übersetzer, Hauptfenster |
| `colorschemewatcher.*` | Hell/Dunkel-Einstellung des Desktops über das XDG-Desktop-Portal, meldet Änderungen (Kopie aus NiriSettingsM) |
| `updatechecker.*` | Update-Prüfung: neuestes Release über die GitHub-API (`releases/latest`, Tag gegen `PROJECT_VERSION`), höchstens einmal am Tag; der Knopf „Update“ öffnet die Release-Seite (der AppImage-Name trägt die Version, daher kein fester Download-Link). Aus MovieMetaM |
| `mainwin.*`, `initialize.cpp` | Hauptfenster: Scannersuche, Optionen, Scannen, Knopfzustände; `initialize.cpp` baut Werkzeugleisten, Formular, Verbindungen und stellt das Fensterlayout wieder her |
| `settingsmanager.*` | Singleton: alle Einstellungen (INI) und die Optionen des aktuellen Scanners |
| `settingsoverlay.*` | Einstellungs-Popup mit Nase unter dem Einstellungsknopf |
| `statusoverlay.*` | Meldungen über dem Fenster (Suche, Laden, Scannen, Fehler) |
| `pagelistwidget.*` | Seitenliste: eine Spalte, Seitengröße folgt der Breite; auswählen, entfernen, Drag and Drop |
| `pagedelegate.*` | zeichnet eine Seite: Bild, Beschriftung „Seite N“, Auswahl, × beim Überfahren |
| `sanedevice.*` | wie scanimage einen Scanner öffnet: escl-Scanner über das airscan-Backend (Abschnitt 4) |
| `ocr.*` | tesseract: installierte Sprachen und ihre Namen |
| `pdfsavedialog.*` | Dateidialog zum Speichern, Knöpfe „Speichern“ / „Mit OCR speichern“ und OCR-Sprache |
| `pageprocessing.*` | Bearbeitung einer gescannten Seite: Drehung bei Querformat, Ausrichten, Farbmodus, Schwarz/Weiß-Schwellwert, Drehen (später Beschneiden) |
| `previewview.*` | große Vorschau (`QGraphicsView`), passt das Bild in die Ansicht ein |
| `helppage.*`, `twolinedelegate.*` | Hilfeseite (Markdown in `help/<de\|en>/`, `Help.qrc`); Kopien aus MovieMetaM, an `UiColors` angepasst |
| `uicolors.h` | Flächenfarben, nur aus Window/WindowText abgeleitet; die Palette der App (`applyStandardPalette`) |
| `toolbarm.*` | Werkzeugleiste; Kopie aus `../libraries/toolbarm` (Master dort, Version 1.7.0) |
| `globals.h` | leer (Kandidat zum Aufräumen) |
| `tools/build-linux.sh` | baut das AppImage und die Release-Dateien (Abschnitt 10) |
| `tools/help-to-html.py` | Dokumentationsseiten für die Website aus `help/<de\|en>/*.md` |
| `ressources/linux/` | Startmenü-Eintrag (`.desktop`) und 256-px-Icon für Linux |

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

### Öffnen: escl-Scanner über airscan (`sanedevice.*`)

Das escl-Backend von SANE 1.4 holt **nur die erste Seite eines Einzug-Auftrags**: Der Scanner
zieht den ganzen Stapel in einem Auftrag ein, escl startet für Seite 2 einen neuen Auftrag und
bekommt „out of documents“ (beobachtet bei HP und Brother). Das airscan-Backend holt jede Seite des
Auftrags. Deshalb öffnet `SaneDevice::forDevice()` escl-Scanner über airscan, wenn `sane-airscan`
installiert ist:

- Eine eigene SANE-Konfiguration in `~/.cache/SpeckenbachNet/PDFDocumentScannerM/sane/`:
  `dll.conf` nur mit `airscan`, `airscan.conf` mit dem Scanner über seine eSCL-Adresse und
  `discovery = disable`. Wird vor jedem `--help` und Scan geschrieben.
- scanimage läuft mit `SANE_CONFIG_DIR` darauf (nur unsere Prozesse; die System-Konfiguration
  bleibt unberührt) und `-d airscan:e0:Scanner` – immer derselbe Name, das Öffnen braucht keine
  Suche (0,04 s). Die eigenen IDs von airscan (`airscan:e0:…`, `w3`) ändern sich bei jeder Suche.
- Gespeichert bleibt die escl-ID. Die eSCL-Adresse kommt von `airscan-discover`
  (`SettingsManager::scannerUrls`), für den gespeicherten Scanner auch aus der INI
  (`Scanner/DeviceUrl`); sonst `http://IP:Port/eSCL/`.
- Ohne `sane-airscan` und bei anderen Backends: wie bisher.
- airscan meldet `*unset*` als Standard des Scan-Zwecks; der Parser nimmt stattdessen „Document“
  (die Combo könnte `*unset*` nicht anzeigen).

### Optionen

`getScannerCapabilities()` ruft `scanimage -d <id> --help` auf und liest `--resolution`,
`--mode`, `--source`, `--scan-intent` (Werteliste und Standard in `[...]`). Bereiche wie
`50..1200` werden zu den üblichen Auflösungen darin (`expandResolutionValues`). Optionen mit
`[inactive]` werden übersprungen. Die größte Scanfläche kommt aus `-x 0..215.9mm` /
`-y 0..297.011mm` (`scanAreaMaxWidth/Height`); sie hängt von der Quelle ab (HP M277dw: Flachbett
297 mm lang, Einzug 381 mm) und gilt nur für die Standardquelle (`scanAreaSource`). Ein gespeicherter Wert, den der Scanner nicht anbietet, wird
durch seinen Standard ersetzt – außer dem Farbmodus: Der gehört der App (nächster Abschnitt),
`--mode` entscheidet nur, womit gescannt wird.

---

## 5. Scannen und Seiten

### Scannen (`MainWin::startScan()`)

- `scanimage` mit `--device-name` und nur den Optionen, die der Scanner gemeldet hat (sonst
  bricht `scanimage` ab), `-x`/`-y` aus dem Seitenformat, `--format=png`.
- Die Scanfläche ist immer hochkant, auch bei Querformat: Das Blatt liegt hochkant auf dem Glas
  oder im Einzug (A4 quer ist breiter als das Glas). Jede Seite wird danach um 90° gegen den
  Uhrzeigersinn gedreht (`convertScannedPage(…, landscape)`).
- `-x`/`-y` werden beim Scannen mit `scanAreaSource` auf die größte Scanfläche gekürzt: airscan
  macht das selbst, escl gibt zu große Werte ungeprüft weiter.
- Einzug (Quelle enthält ADF/Feeder/Duplex, `isAdfSource`): `--batch=page_%03d.png`.
  Flachbett: eine Seite (`--batch-count=1`).
- Zielordner `/tmp/scan_job_<yyyyMMdd_hhmmss>/`. Ein Timer (1,5 s) holt fertige Seiten schon
  während des Scans ab (erst kopieren, damit nie eine halb geschriebene Datei geladen wird).
- Exit-Codes 5 und 7 nach mindestens einer Seite heißen „Einzug leer“, kein Fehler.
- Scheitert der Scan, wird die letzte Zeile von stderr angezeigt.
- **Abbrechen:** Während des Scans heißt der Scan-Knopf „Abbrechen“ (der einzige benutzbare
  Knopf), Esc macht dasselbe (`cancelScan()`). `QProcess::terminate()` schickt SIGTERM; scanimage
  antwortet auf das erste mit `sane_cancel` (der Scanner erfährt es, escl löscht den
  Scan-Auftrag). Läuft es nach 5 s noch: `kill()`. Fertige Seiten bleiben; eine halb
  geschriebene letzte Seite wird gelöscht (die Abschlussprüfung lädt jede Datei). Die Meldung
  ist „Scan abgebrochen“, kein Fehler – scanimage endet mit einem Fehlercode. `endScan()` stellt
  Knopf und Zustände zurück.

### Farbmodus (`pageprocessing.*`)

Die App hat drei feste Modi; die Namen des Scanners (`Color`, `Colour`, `24bit Color`, `Gray`,
`True Gray`, `Lineart`, …) entscheiden nur, womit gescannt wird.

| Combo | INI | gescannt mit | danach in der App |
|---|---|---|---|
| Farbe | `Color` | Farbe | nichts |
| Graustufen | `Gray` | Grau, sonst Farbe | Farbe → Grau |
| Schwarz/Weiß | `BlackWhite` | Grau, sonst Farbe | Grau → **1 Bit** mit automatischem Schwellwert |

- `scannerModeFor()` wählt den Wert des Scanners: zuerst ein exakter Name, dann einer, der das
  Wort enthält, aber keine gerasterte Variante (`Gray[Error Diffusion]`). Gar kein `--mode`: der
  Standard des Scanners.
- Schwarz/Weiß macht die App bei jedem Scanner selbst, auch bei denen mit `Lineart` (Brother):
  Das Ergebnis sieht überall gleich aus, Scanner setzen den Schwellwert grob.
- **Umgewandelt wird sofort, wenn eine Seite eintrifft** (`convertScannedPage()`, bevor sie in
  die Liste kommt): Liste, Vorschau, Drehen und PDF sehen alle die fertige Seite, später ist
  nichts mehr zu tun.
- Schwellwert: **Otsu** pro Seite (`otsuThreshold()`), passt sich an weißes oder graues Papier
  an. Grenze: ein Schwellwert für die ganze Seite; Schatten (Buchfalz) oder ungleichmäßiges
  Licht bräuchten ein örtliches Verfahren (Sauvola) – erst, wenn es in der Praxis stört.
- Das Grau-Original einer Schwarz/Weiß-Seite bleibt als `gray_page_001.png` (`originalPath()`;
  das Präfix hält es aus dem Filter `page_*.png` heraus) für einen späteren neuen Schwellwert.
  Drehen dreht es mit, Entfernen einer Seite löscht es.
- Drehen behält das Bildformat (`PageProcessing::rotatedImage()`): Qt macht aus einem
  1-Bit-Bild beim Drehen 32 Bit.
- Ältere Einstellungen mit `Lineart` werden als `BlackWhite` gelesen.

### Ausrichten (`pageprocessing.*`)

Einstellung `Scanner/Straighten` (Häkchen „Dokument ausrichten“, Standard aus), gelesen beim
Start des Scans. Gemacht in `convertScannedPage()` gleich nach dem Eintreffen der Seite: Drehung
bei Querformat → Ausrichten → Farbmodus. Schwarz/Weiß-Seiten werden in Grau ausgerichtet und
danach mit dem Schwellwert umgewandelt (ein gedrehtes 1-Bit-Bild bekommt ausgefranste Buchstaben).

- `skewAngle()`: Seite auf 1000 px Breite verkleinert (etwa 100 dpi), dunkle Punkte nach Otsu,
  äußere 3 % ausgelassen (Randschatten). Für jeden Winkel wird jeder Punkt in der Zeile
  `y − x · tan(Winkel)` gezählt; es gewinnt der Winkel, dessen Zeilenprofil die größte Summe der
  quadrierten Nachbar-Unterschiede hat (Textzeilen fallen auf wenige Zeilen). Suche ±5°: grob in
  0,25°-Schritten, dann 0,025° um den besten.
- Keine Drehung, wenn weniger als 0,2 % oder mehr als 35 % der Seite dunkel sind (leere Seite,
  Foto), der beste Wert unter dem 1,5-Fachen des Medians der groben Werte liegt (keine klaren
  Zeilen) oder der Winkel unter 0,2° liegt.
- `straightenedImage()`: in 32 Bit mit `SmoothPixmapTransform` gemalt, um −Winkel um die Mitte
  gedreht, gleiche Größe, frei werdende Ecken weiß; zurück ins Format der Seite, Auflösung
  bleibt.
- Der Winkel kommt über `convertScannedPage(..., &straightenedBy)` zurück und steht im Eintrag
  (`PageListWidget::kStraightenedRole`); die Beschriftung lautet „Seite 3 · ausgerichtet 2,8°“
  (`setPageLabel()`, auch beim Neunummerieren), der ganze Text als Tooltip, der Delegate kürzt
  ihn mit „…“.
- Gemessen mit künstlichen A4-Seiten mit 300 dpi (Neigung 0,3° … 4,9°, beide Richtungen): Winkel
  auf 0,025° genau, etwa 60 ms zum Finden und 70 ms zum Drehen. Mit echten Scans noch nicht
  getestet.

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
- Signal `pagesChanged()` nach Hinzufügen, Entfernen, Verschieben und Drehen;
  `MainWin::updateButtons()` folgt ihm und der Auswahl. Die Vorschau folgt `currentItemChanged`
  (`MainWin::showCurrentPage`) und `pageImagesChanged()` (eine gedrehte Seite wird neu geladen).
- Jede Seite merkt sich die Auflösung ihres Scans (`PageListWidget::kDpiRole`): Die Einstellung
  kann sich zwischen zwei Scans ändern, die Seitengröße der PDF braucht die richtige.
- Drehen: die ausgewählten Seiten um 90° im Uhrzeigersinn; die PNG-Datei selbst wird neu
  geschrieben, so brauchen Liste, Vorschau und PDF keinen zusätzlichen Zustand.
- Leeren: alle Seiten, nach einer Rückfrage (die Bilddateien werden gelöscht).

### Als PDF speichern (`MainWin::savePdf()`)

- Ausgewählte oder alle Seiten in der Reihenfolge der Liste (`PageListWidget::pages()`).
- Dateidialog: Name `Scan_<yyyy-MM-dd_HHmm>.pdf`, vorgeschlagen im Ordner des letzten Speicherns
  (INI `Save/LastFolder`; vor dem ersten Speichern der Dokumente-Ordner).
- `writePdf()` benutzt `QPdfWriter` (Qt GUI, keine zusätzliche Abhängigkeit): eine Seite pro
  Bild, das Bild füllt die Seite. Die Seitengröße ergibt sich aus Pixeln und Scan-Auflösung
  (`pageLayoutFor()`, `FuzzyMatch` macht aus 209,97 × 297,01 mm A4); Querformat-Seiten bekommen
  ein Querformat-Layout, weil `QPageSize` allein immer hochkant ist.
- Der Dateidialog ist der eigene von Qt (`PdfSaveDialog`, `DontUseNativeDialog`): Nur der nimmt
  zusätzliche Knöpfe. Qts Buttonleiste (senkrecht, nach den Regeln des Desktops sortiert) ist
  versteckt; eigene Knöpfe in fester Reihenfolge: **Abbrechen | Mit OCR speichern | Speichern**
  (Standard, Eingabetaste). Sie folgen Qts verstecktem Speichern-Knopf (`EnabledChange`: ohne
  Dateinamen aus) und rufen `QFileDialog::accept()` (prüft den Namen, fragt vor dem
  Überschreiben). Darüber die OCR-Sprache, voreingestellt aus den Einstellungen
  (`Scanner/OcrLanguage`); eine Änderung dort gilt nur für dieses Speichern und wird nicht
  gemerkt. Ohne gespeicherte Sprache die des Systems, sonst die erste (`Ocr::fillLanguageCombo`).
- Aussehen: Die Zeile Dateityp ist ausgeblendet (nur PDF), Ränder 20 px, Dateiname / Ordner /
  Sprache rund, Seitenleiste und Dateiliste als runde Boxen mit 26 px hohen Zeilen
  (`UiColors::applyInputStyle`, `applyComboStyle`, `applyBoxStyle`, `listItemRules`, übernommen
  aus `themecolors.h` von NiriSettingsM). Qts Teile werden über ihren Objektnamen gefunden
  (`fileNameEdit`, `lookInCombo`, `sidebar`, `listView`, `treeView`); fehlt einer, bleibt Qts
  Aussehen. Beim Wechsel Hell/Dunkel neu gesetzt (die Stylesheets enthalten feste Farben).
  Größe und Ansicht werden gemerkt.
- **OCR läuft beim Speichern** (Variante A): tesseract schreibt die PDF aus denselben Bildern statt
  `QPdfWriter` (`tesseract liste.txt <ziel>.part -l deu pdf`, Bild und unsichtbarer Text in
  einem Durchgang). Seitengröße aus der Auflösung in der PNG (`pHYs`, schreibt scanimage, Qt
  behält sie beim Umwandeln und Drehen). Gemessen: Die PDF ist nicht größer als mit `QPdfWriter`,
  etwa 4 s pro A4-Seite mit 300 dpi.
- tesseract übernimmt eine PNG unverändert in die PDF (Flate), andere Dateien kodiert es selbst
  (Leptonica: 1 Bit → CCITT G4). Deshalb bekommt es Schwarz/Weiß-Seiten als unkomprimierte
  1-Bit-TIFF (`PageProcessing::writeBlackWhiteTiff()`, eigener Schreiber mit den
  Auflösungs-Tags – Qts TIFF-Schreiber ist ein Plugin und kann nur LZW). Gemessen mit zwei echten
  Textseiten: 110 KB → 70 KB, gleicher Text, gleiches Bild. Kleiner ginge mit JBIG2 (PDFScanner
  auf dem Mac: 45 KB) – braucht jbig2enc und einen eigenen PDF-Schreiber, später.
- `writeOcrPdf()` läuft in einer eigenen `QEventLoop`: Das Fenster bleibt bedienbar, die
  Statusmeldung zeigt „Seite n von m“ (aus tesseracts Zeilen `Page n : …`), **Esc** bricht ab.
  Die Seitenliste ist so lange gesperrt (die Bilder dürfen sich nicht ändern), Schließen wartet.
  tesseract schreibt `<ziel>.part.pdf`, das erst am Ende umbenannt wird: Abbruch oder Fehler
  lassen eine vorhandene Datei unberührt.
- Ausrichten geschieht gleich nach dem Scan (siehe Ausrichten), Beschneiden ebenso; beim
  Speichern bleibt dann nur OCR → PDF. Vorerst verworfen: OCR gleich nach
  dem Scan mit markierbarem Text in der Vorschau (Variante B) – Drehen, Ausrichten und
  Beschneiden müssten dann mit erkanntem Text umgehen.
- Ohne OCR blockiert das Schreiben das Fenster; ein `processEvents()` lässt die Meldung „PDF wird
  gespeichert...“ vorher erscheinen.

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
Aller Zugriff läuft über `SettingsManager`, außer den Update-Schlüsseln (`UpdateChecker`, wie in
MovieMetaM).

| Schlüssel | Inhalt |
|---|---|
| `Scanner/DeviceId`, `Scanner/DeviceName`, `Scanner/DeviceUrl` | der Scanner des Benutzers (nur bei seiner Wahl geschrieben) |
| `Scanner/ScanningUnit`, `Resolution`, `ScanIntent` | Scanner-Optionen |
| `Scanner/ColorMode` | `Color`, `Gray` oder `BlackWhite` (die Modi der App) |
| `Scanner/PageOrientation`, `PageFormat` | Seite |
| `Scanner/OcrLanguage` | Voreinstellung der OCR-Sprache im Speicherdialog, tesseract-Code (`deu`, `eng`) |
| `Save/DialogGeometry`, `Save/DialogState` | Größe und Ansicht des Speicherdialogs (`saveGeometry()` / `QFileDialog::saveState()`, geschrieben in `done()`) |
| `Window/Geometry`, `Window/Splitter` | `saveGeometry()` / `QSplitter::saveState()`, geschrieben in `closeEvent` |
| `Save/LastFolder` | Ordner der zuletzt gespeicherten PDF |
| `Scanner/Straighten` | neue Scans ausrichten (Häkchen „Dokument ausrichten“) |
| `General/CheckUpdates` | bei GitHub nach einer neuen Version fragen (Standard an) |
| `General/LastUpdateCheck`, `General/LatestVersion` | Tag der letzten Anfrage und die gefundene Version: höchstens eine Anfrage am Tag, eine neuere Version erscheint beim nächsten Start wieder ohne Anfrage |

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
- `main.cpp` lädt die erste Systemsprache, für die es eine Übersetzung gibt, sonst Englisch, und
  Qts eigene Übersetzung `qtbase` in derselben Sprache (Standardknöpfe, Dateidialog).
- Hilfetexte: `help/de/*.md`, `help/en/*.md` (`Help.qrc`).

---

## 8. UI-Bausteine und Stil

- **Hell/Dunkel:** Stil Fusion. `ColorSchemeWatcher` liest `color-scheme` aus dem
  XDG-Desktop-Portal und meldet Änderungen; `main.cpp` setzt es mit
  `QStyleHints::setColorScheme()` und danach `UiColors::applyStandardPalette()`: immer Fusions
  eigene helle oder dunkle Palette (ein Plattform-Theme wie gtk3 hat Farben nur für einen Modus,
  gemischt mit denen von Fusion sahen sie nach mehrmaligem Umschalten anders aus), mit lesbarer
  Auswahlfarbe. Funktioniert mit jedem `QT_QPA_PLATFORMTHEME` und im AppImage gleich.
- **Farben werden nur aus Window/WindowText abgeleitet** (`uicolors.h`, `UiColors::mix`).
  Ursprünglich, weil mit der gtk3-Palette Window, Button und Base gleich und Light/Mid/Dark fast
  gleich sind; mit der Fusion-Palette weiterhin der sichere Weg.
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
- Keine Erwähnung von Claude oder `CLAUDE.md` in Commits; `CLAUDE.md` und `.claude/` stehen in
  `.gitignore`.
- Änderungen ohne App-Code (Doku, Release-Dateien, Skripte) bekommen keine neue Version.

### Veröffentlichung

1. Version in `CMakeLists.txt` setzen.
2. `CHANGELOG.md`: neuer Abschnitt oben (Englisch, für Benutzer). `README.md`: Abschnitt „What's
   new in X.Y.Z“ (sie ist die Beschreibung des GitHub-Releases).
3. `tools/build-linux.sh` – Ergebnis in `build/` (nicht in Git):
   - `PDFDocumentScannerM-X.Y.Z-linux.AppImage`
   - `updates.xml` (Version, Datum des Builds, Release- und Download-Adresse; festes Format, nur
     Linux)
   - `CHANGELOG.md`, `README.md` (Kopien)
   - `doc/documentation-en.html`, `doc/documentation-de.html` (aus der Hilfe, für die Website)
4. Commit, Tag `vX.Y.Z` (einfacher Tag), `main` und Tag pushen.
5. GitHub-Release zum Tag: das AppImage als Datei, `README.md` als Beschreibung.

### Linux-Paket (`tools/build-linux.sh`)

Ablauf wie in MovieMetaM: ein Podman-Container mit **Ubuntu 22.04** (glibc 2.35, damit das
AppImage auch auf älteren Systemen startet), Qt 6.12 über aqtinstall, CMake aus pip, linuxdeploy
mit seinem Qt-Plugin. Von Hand dazu: das Wayland-Plugin, das Plattform-Theme des Desktop-Portals
(Dateidialog von „PDF speichern“) und `libOpenGL.so.0`. Erster Lauf: etwa 1,8 GB in
`build/linux-cache`. Braucht nur Podman (ohne root) und Python 3 auf dem Rechner.

Hell/Dunkel im AppImage: Ein in der Umgebung gesetztes Theme (z. B. `QT_QPA_PLATFORMTHEME=gtk3`
unter niri) fehlt dort. Das spielt keine Rolle mehr: Die App fragt das Desktop-Portal selbst
(`ColorSchemeWatcher`, Abschnitt 8) und nimmt die Fusion-Palette. Qts eigene Übersetzungen
(`qtbase_*.qm`) packt linuxdeploy mit ins Paket (`usr/translations`).

- Der Paketname trägt die Version (`PDFDocumentScannerM-0.5.1-linux.AppImage`); das Skript liest
  sie aus `CMakeLists.txt`.
- `main.cpp` setzt den Desktop-Dateinamen (`net.speckenbach.PDFDocumentScannerM`): Unter Wayland
  verbindet er das Fenster mit dem Startmenü-Eintrag und seinem Icon.
- Die Update-Prüfung braucht HTTPS: Qts TLS-Plugin (`plugins/tls`, OpenSSL-Backend) muss im
  AppImage sein. Nach dem ersten Build damit (0.8.0) prüfen, ob der Knopf auch im AppImage
  erscheint.
- `scanimage`, `airscan-discover` und `tesseract` sind **nicht** im Paket; sie kommen vom
  System (siehe Voraussetzungen). Noch nicht getestet: ob die App aus dem AppImage sie ohne
  Probleme aufruft.

### Doku für die Website (`tools/help-to-html.py`)

Macht aus der Hilfe die Dokumentationsseiten (dasselbe HTML wie die MovieMetaM-Doku,
Joomla-Beitrag). Ein Abschnitt pro Hilfedatei, in der Reihenfolge der Hilfeseite; `#` →
Abschnittstitel, `##` → `h3`, `>` → Hinweis (`mm-story`), Bilder → `figure` mit Unterschrift
(`:/images/help/` → `images/software/pdfdocumentscannerm/help/`), fetter Text, der der Titel eines
anderen Themas ist → Link dorthin (nicht innerhalb dieses Themas). Die Versionszeile der About-Seite fällt weg. Adressen der
Seiten: `en/pdfdocumentscannerm/documentation` und `de/pdfdocumentscannerm/dokumentation`
(`alias` in `TEXTS`; der Sprach-Button verlinkt auf den Alias der anderen Sprache).


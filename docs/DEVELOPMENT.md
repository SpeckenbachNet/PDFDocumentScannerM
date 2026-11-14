# PDFDocumentScannerM – Developer Documentation

[Deutsch](DEVELOPMENT.de.md)

As of version 0.5.2 (November 2026). Code, comments, commit messages and the source texts of the
translations are English. This document exists in English and German; change both together.

---

## 1. Overview

PDFDocumentScannerM scans documents with network scanners and (in the end) saves them as PDF
(Qt 6 Widgets, C++17, CMake, Linux). It does not talk to SANE directly but runs the command line
tools `airscan-discover` and `scanimage` (`QProcess`).

**State**

| Feature | State |
|---|---|
| Scanner search (`airscan-discover`, fallback `scanimage -L`), saved scanner remembered | done |
| Scanner options (source, color mode, scan intent, resolution) from `scanimage --help` | done |
| Scan flatbed (one page) and feeder (batch), pages appear while scanning, cancel (button, Esc) | done |
| Page list: select, remove (×, Delete, context menu), reorder by drag and drop | done |
| Large preview of the current page with zoom | done |
| Window size and splitter remembered | done |
| Help page | frame done, texts mostly "Coming soon" |
| Save as PDF (selected / all pages, without OCR), rotate, clear | done |
| Fake duplex | open (button exists, not connected) |
| OCR, straighten, crop (check boxes in the settings) | open (not bound) |

**Flow**

```
 Start ──► scanner search ──► options of the chosen scanner ──► Scan
                                                                   │
            scanimage writes page_001.png, page_002.png, ... to /tmp/scan_job_<time>/
                                                                   │
                         page list (new page = current) ──► preview
```

The window has two pages (`stackedWidget`, each with its own toolbar, `showPage()` switches):
the main page (toolbar, page list on the left, preview on the right, splitter between them) and
the help page. The settings are a popup (`SettingsOverlay`) below the settings button.

---

## 2. Setup and Build

### Prerequisites

Tested on CachyOS (Arch). The build packages are named as in Arch; the runtime tables also list
other distributions.

**Build**

| Package | Tested | For |
|---|---|---|
| `cmake` | 4.4.4 | build (at least 3.16) |
| `qt6-base`, `qt6-svg` | 6.12.0 | Widgets, SVG icons |
| `qt6-tools` | 6.12.0 | `lupdate` / `lrelease` for the `.ts` files |

**Runtime**

| For | Tested (Arch) | Arch/CachyOS | Debian/Ubuntu | Fedora/RHEL | openSUSE |
|---|---|---|---|---|---|
| `scanimage` (options, scanning) and the `escl` backend | 1.4.0 | `sane` | `sane-utils` | `sane-backends` | `sane-backends` |
| `airscan-discover`, the fast scanner search | 0.99.38 | `sane-airscan` | `sane-airscan` | `sane-airscan` | `sane-airscan` |
| mDNS (service `avahi-daemon` running), finds the network scanners | 0.9rc5 | `avahi` | `avahi-daemon` | `avahi` | `avahi` |
| OCR – **not built in yet**, planned | 5.5.3 | `tesseract` | `tesseract-ocr` | `tesseract` | `tesseract-ocr` |

Without `sane-airscan` the app falls back to `scanimage -L` (about 10 s, often misses devices).

**OCR language data:** one package for every OCR language that can be chosen (German and
English). Under Arch English is **not** included in `tesseract` (it only pulls in `osd` and any
one language package).

| Language | Arch/CachyOS | Debian/Ubuntu | Fedora/RHEL | openSUSE |
|---|---|---|---|---|
| German | `tesseract-data-deu` | `tesseract-ocr-deu` | `tesseract-langpack-deu` | `tesseract-ocr-traineddata-german` |
| English | `tesseract-data-eng` | `tesseract-ocr-eng` | `tesseract-langpack-eng` | `tesseract-ocr-traineddata-english` |

Only CachyOS is tested; the package names of the other distributions are not tested.

**Not needed:** `hplip`. Its `hpaio` backend does not scan the HP M277dw; the app uses `escl`.

### Scanner and network

None of this follows from the source code:

- The scanner must offer **eSCL** (AirScan) in the network and have an **IPv4 address**. The
  `escl` backend of SANE 1.4 cannot open IPv6 URLs.
- A **fixed IP** (DHCP reservation in the router) is recommended: the device ID contains the
  address. If it changes, the saved scanner is "gone" and has to be chosen again.
- `escl` must be enabled in `/etc/sane.d/dll.conf` (the default).
- **HP Color LaserJet MFP M277dw:** in the printer's web interface the redirect from HTTP to
  HTTPS must be switched off, otherwise `escl` fails.
- **Brother MFC-L2710DW:** works without changes. `airscan-discover` sometimes lists it only via
  WSD (about 1 in 6 runs); then the direct query of the saved scanner takes over (section 4).

### Build

Easiest: open `CMakeLists.txt` in **Qt Creator**, choose a kit, build and run.

On the command line:

```
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

Notes:

- **Create new source files first, then add them to `CMakeLists.txt`.** Qt Creator reconfigures
  at once; if a file is missing, its build folder breaks.
- Defines from `CMakeLists.txt`: `PROJECT_NAME`, `PROJECT_VERSION`, `COMPANY_NAME`.
- After changing `CMakeLists.txt`, run CMake again (Qt Creator usually does it by itself).

---

## 3. Project Structure

| File | Content |
|---|---|
| `main.cpp` | application and organization name, INI format, translator, main window |
| `mainwin.*`, `initialize.cpp` | main window: scanner search, options, scanning, button states; `initialize.cpp` builds toolbars, form, connections and restores the window layout |
| `settingsmanager.*` | singleton: all settings (INI) and the options of the current scanner |
| `settingsoverlay.*` | settings popup with the nose below the settings button |
| `statusoverlay.*` | messages over the window (search, loading, scanning, errors) |
| `pagelistwidget.*` | page list: one column, page size follows the width; select, remove, drag and drop |
| `pagedelegate.*` | draws a page: image, label "Page N", selection, × on hover |
| `pageprocessing.*` | processing of a scanned page: color mode, black/white threshold (later straighten, crop) |
| `previewview.*` | large preview (`QGraphicsView`), fits the image into the view |
| `helppage.*`, `twolinedelegate.*` | help page (Markdown in `help/<de\|en>/`, `Help.qrc`); copies from MovieMetaM, adapted to `UiColors` |
| `uicolors.h` | surface colors, derived from Window/WindowText only |
| `toolbarm.*` | toolbar; copy from `../libraries/toolbarm` (master there, version 1.7.0) |
| `globals.h` | empty (cleanup candidate) |
| `tools/build-linux.sh` | builds the AppImage and the release files (section 10) |
| `tools/help-to-html.py` | documentation pages for the website from `help/<de\|en>/*.md` |
| `ressources/linux/` | start menu entry (`.desktop`) and 256 px icon for Linux |

---

## 4. Scanners

### Search

`MainWin::searchForScanners()` runs at start and with the refresh button.

- If `airscan-discover` is installed (`QStandardPaths::findExecutable`), it is used: 1–3 s,
  finds both test scanners reliably. Otherwise `scanimage -L` (about 10 s, its mDNS search often
  misses a device).
- `saveAirscanScanners()` uses only lines with `, eSCL` and an IPv4 address and turns them into
  **escl** device IDs:
  `Brother MFC-L2710DW series = http://192.168.178.23:80/eSCL/, eSCL` →
  `escl:http://192.168.178.23:80`, shown as `escl: Brother MFC-L2710DW series (192.168.178.23)`.
- Why not the airscan IDs? `airscan:e0:…`, `airscan:w3:…` contain a running number that changes
  with every search; they cannot be stored. The escl ID is the URL and stays the same.
- `saveScanners()` parses `scanimage -L` (fallback); webcams (`v4l`) and IPv6 escl URLs are
  skipped.

### Choosing and remembering

- `SettingsManager` keeps two scanners: `savedDeviceId()` (the user's choice, INI
  `Scanner/DeviceId` + `Scanner/DeviceName`) and `deviceId()` (used in this session).
- Only `chooseDevice()` writes the INI – called when the user picks a scanner in the overlay.
  `setDeviceId()` changes the session only.
- `chooseScanner()`: if the search missed the saved scanner, it is still put into the list
  (with the saved name) and queried **directly**: `escl` opens the URL without a search
  (0.05–0.3 s measured). `m_probingSavedScanner` marks this.
- If it does not answer either (`getScannerCapabilities` fails), `useStandInScanner()` removes it
  and takes the first found scanner for this session. The INI keeps the user's choice; the next
  search tries it again.

### Options

`getScannerCapabilities()` runs `scanimage -d <id> --help` and reads `--resolution`, `--mode`,
`--source`, `--scan-intent` (value list and default in `[...]`). Ranges like `50..1200` become
the common resolutions within (`expandResolutionValues`). Options marked `[inactive]` are
skipped. A saved value the scanner does not offer is replaced by its default – except the color
mode: it is the app's own (next section), `--mode` only decides what to scan with.

---

## 5. Scanning and Pages

### Scanning (`MainWin::startScan()`)

- `scanimage` with `--device-name`, and only the options the scanner reported (otherwise
  `scanimage` aborts), `-x`/`-y` from the page format (orientation swaps them), `--format=png`.
- Feeder (source contains ADF/Feeder/Duplex, `isAdfSource`): `--batch=page_%03d.png`.
  Flatbed: one page (`--batch-count=1`).
- Target folder `/tmp/scan_job_<yyyyMMdd_hhmmss>/`. A timer (1.5 s) picks up finished pages while
  scanning (copy first, so a half written file is never loaded).
- Exit codes 5 and 7 after at least one page mean "feeder empty", not an error.
- The last line of stderr is shown if the scan fails.
- **Cancel:** while scanning the scan button reads "Cancel" (the only usable button), Esc does
  the same (`cancelScan()`). `QProcess::terminate()` sends SIGTERM; scanimage answers the first
  one with `sane_cancel` (the scanner is told, escl deletes the scan job). Still running after
  5 s: `kill()`. Complete pages stay; a half written last page is deleted (the final check
  loads every file). The message is "Scan cancelled", not an error – scanimage ends with an
  error code. `endScan()` puts button and states back.

### Color mode (`pageprocessing.*`)

The app has three fixed modes; the scanner's names (`Color`, `Colour`, `24bit Color`, `Gray`,
`True Gray`, `Lineart`, ...) only decide what is scanned with.

| Combo | INI | Scanned with | Then in the app |
|---|---|---|---|
| Color | `Color` | color | nothing |
| Grayscale | `Gray` | gray, else color | color → gray |
| Black/white | `BlackWhite` | gray, else color | gray → **1 bit** with an automatic threshold |

- `scannerModeFor()` picks the scanner value: an exact name first, then one containing the word,
  but no dithered variant (`Gray[Error Diffusion]`). No `--mode` at all: the scanner's default.
- Black/white is done by the app for every scanner, also for those with `Lineart` (Brother):
  the result looks the same everywhere, scanners threshold coarsely.
- **Converted right after each page arrives** (`convertScannedPage()`, before it enters the
  list): list, preview, rotating and PDF all see the final page, nothing is left to do later.
- Threshold: **Otsu** per page (`otsuThreshold()`), adapts to white or gray paper. Limit: one
  threshold for the whole page; shadows (book fold) or uneven light would need a local method
  (Sauvola) – only if it disturbs in practice.
- The gray original of a black/white page stays as `gray_page_001.png` (`originalPath()`; the
  prefix keeps it out of the `page_*.png` filter) for a later new threshold. Rotating turns it
  along, removing a page deletes it.
- Rotating keeps the image format (`rotatedImage()` in `pagelistwidget.cpp`): Qt turns a 1 bit
  image into 32 bit while rotating.
- Older settings with `Lineart` are read as `BlackWhite`.

### Page list (`PageListWidget`, `PageDelegate`)

- ListMode, one column. Pages fill the width (A4 ratio). If they do not fit the height, they
  shrink at most by the width of the scroll bar before the bar appears (`updateItemSizes`).
- Select: click, Ctrl/Shift + click, Ctrl+A. Remove: × on hover, Delete/Backspace, context
  menu "Remove n pages" – **the image file is deleted too**. Labels are renumbered.
- Reorder: drag and drop (`InternalMove`). ListMode, because IconMode places dropped items freely.
- **Auto scroll while dragging is our own:** Qt's reads `QCursor::pos()`, which does not move
  during a drag under Wayland. A timer scrolls while the drag position (from the drag events) is
  within 40 px of the top or bottom edge (`kDragScrollZone`, `kDragScrollMaxStep`,
  `kDragScrollInterval`).
- Signal `pagesChanged()` after adding, removing, moving and rotating; `MainWin::updateButtons()`
  follows it and the selection. The preview follows `currentItemChanged`
  (`MainWin::showCurrentPage`) and `pageImagesChanged()` (a rotated page is loaded again).
- Every page keeps the resolution of its scan (`PageListWidget::kDpiRole`): the setting may
  change between two scans, the PDF page size needs the right one.
- Rotate: the selected pages 90° clockwise; the PNG file itself is rewritten, so list, preview
  and PDF need no extra state.
- Clear: all pages, after a question (the image files are deleted).

### Saving as PDF (`MainWin::savePdf()`)

- Selected or all pages in list order (`PageListWidget::pages()`).
- File dialog: name `Scan_<yyyy-MM-dd_HHmm>.pdf`, suggested in the folder of the last save
  (INI `Save/LastFolder`; before the first save the documents folder).
- `writePdf()` uses `QPdfWriter` (Qt GUI, no extra dependency): one page per image, the image
  fills the page. The page size follows from pixels and scan resolution (`pageLayoutFor()`,
  `FuzzyMatch` turns 209.97 × 297.01 mm into A4); landscape pages get a landscape layout,
  because `QPageSize` alone is always portrait.
- **OCR (planned) happens when saving** (decision: variant A). Then tesseract writes the PDF
  from the same images instead of `QPdfWriter` (`tesseract list.txt out -l deu pdf`, image and
  invisible text in one run). Order then: straighten → crop → OCR → PDF. Rejected for now: OCR
  right after scanning with selectable text in the preview (variant B) – rotating, straightening
  and cropping would have to work on recognized text.
- Writing blocks the window; a `processEvents()` lets the message "Saving PDF..." appear first.

### Preview (`PreviewView`)

- A new page shows the whole page; in this fit mode the page follows every resize. The splitter
  gives a wider window to the preview, the page list keeps its width (stretch factors 0 / 1).
- Zoom: **Ctrl + mouse wheel** (at the mouse), Ctrl + Plus / Minus / 0, double click (whole
  page ↔ 100 %), or the zoom bar in the lower right corner that appears while the mouse is over
  the preview (−, level – click: whole page –, +). The plain wheel scrolls, so it is not used for
  zooming.
- Zoomed, resizing keeps the zoom level, dragging moves the page (`ScrollHandDrag`). Range: whole
  page up to 8 × of it, at least 100 %.
- The fit scale is computed from `maximumViewportSize()` instead of `fitInView()`: that measures
  the viewport while the scroll bars of the zoomed page are still shown.
- The shortcuts are window shortcuts; they do not fire while the help page is shown (hidden
  widget).

---

## 6. Settings (QSettings)

`main.cpp` sets organization `SpeckenbachNet` (`ORGANIZATION_NAME` in `CMakeLists.txt`), the
application name and the INI format. File: `~/.config/SpeckenbachNet/PDFDocumentScannerM.ini`.
All access goes through `SettingsManager`.

| Key | Content |
|---|---|
| `Scanner/DeviceId`, `Scanner/DeviceName` | the user's scanner (only written on the user's choice) |
| `Scanner/ScanningUnit`, `Resolution`, `ScanIntent` | scanner options |
| `Scanner/ColorMode` | `Color`, `Gray` or `BlackWhite` (the app's modes) |
| `Scanner/PageOrientation`, `PageFormat` | page |
| `Scanner/OcrLanguage` | tesseract code (`deu`, `eng`) |
| `Window/Geometry`, `Window/Splitter` | `saveGeometry()` / `QSplitter::saveState()`, written in `closeEvent` |
| `Save/LastFolder` | folder of the last saved PDF |

First start without saved window: 900 × 600 (`kDefaultSize` in `restoreWindowLayout()`, fits
laptops with 150 % scaling), page list 180 px. Qt widens the window by itself if the toolbar
needs more room. Under Wayland the window position is not restored (the compositor decides).

---

## 7. Translations

- `i18n/PDFDocumentScannerM_de.ts`, `_en.ts`; `qt_add_translations` builds the `.qm` files into
  `i18n/`, `translation.qrc` embeds them.
- Many sources are keys (`SCAN`, `REMOVE_PAGES %n`), so English needs a translation too. Newer
  texts are plain English sentences.
- `main.cpp` loads the first system language with a translation, otherwise English.
- Help texts: `help/de/*.md`, `help/en/*.md` (`Help.qrc`).

---

## 8. UI Building Blocks and Style

- **Colors are derived from Window/WindowText only** (`uicolors.h`, `UiColors::mix`). With the
  gtk3 platform theme Window, Button and Base are equal and Light/Mid/Dark nearly so; colors taken
  from them vanish.
- Page list and preview lie deeper than the window, like the toolbar (`applySurfaceColors`,
  called again on palette changes, so light/dark switches apply live).
- `ToolbarM`: buttons with a fixed size from text, icon and font; `addSpacer()` for groups.
  `applyToolbarStyle()` in `initialize.cpp` sets the shade factors for dark and light.
- Button states in one place: `setBusy()` / `updateButtons()`. All buttons start disabled.

---

## 9. Resources and Licenses

- License: **GPL 3**. Repository: https://github.com/SpeckenbachNet/PDFDocumentScannerM
- Icons: `ressources/icons/*.svg` from the icon library in `../libraries/icons` (`icons.qrc`).

---

## 10. Versions and Commits

- The version is `PROJECT_VERSION` in `CMakeLists.txt`. Every new version is its own commit; the
  first line of the message ends with `(vX.Y.Z)`.
- Commit messages in English (older commits up to v0.3.2 are German).
- No mention of Claude or `CLAUDE.md` in commits; `CLAUDE.md` and `.claude/` are in `.gitignore`.
- Changes without app code (documentation, release files, scripts) get no new version.

### Release

1. Set the version in `CMakeLists.txt`.
2. `CHANGELOG.md`: new section on top (English, for users). `README.md`: section "What's new in
   X.Y.Z" (it is the description of the GitHub release).
3. `tools/build-linux.sh` – result in `build/` (not in Git):
   - `PDFDocumentScannerM-X.Y.Z-linux.AppImage`
   - `updates.xml` (version, date of the build, release and download URL; fixed format, Linux only)
   - `CHANGELOG.md`, `README.md` (copies)
   - `doc/documentation-en.html`, `doc/documentation-de.html` (from the help, for the website)
4. Commit, tag `vX.Y.Z` (lightweight), push `main` and the tag.
5. GitHub release for the tag: the AppImage as file, `README.md` as description.

### Linux package (`tools/build-linux.sh`)

Same procedure as in MovieMetaM: a Podman container with **Ubuntu 22.04** (glibc 2.35, so the
AppImage also starts on older systems), Qt 6.12 with aqtinstall, CMake from pip, linuxdeploy with
its Qt plugin. By hand: the Wayland plugin, the platform theme of the desktop portal (file dialog
of "Save PDF") and `libOpenGL.so.0`. First run: about 1.8 GB in `build/linux-cache`. Needs only
Podman (no root) and Python 3 on the host.

- The name of the package carries the version (`PDFDocumentScannerM-0.5.1-linux.AppImage`); the
  script reads it from `CMakeLists.txt`.
- `main.cpp` sets the desktop file name (`net.speckenbach.PDFDocumentScannerM`): under Wayland it
  ties the window to the start menu entry and its icon.
- `scanimage`, `airscan-discover` (and later `tesseract`) are **not** in the package; they come
  from the system (see Prerequisites). Not tested yet: whether the app started from the AppImage
  runs them without problems.

### Documentation for the website (`tools/help-to-html.py`)

Turns the help into the documentation pages (same HTML as the MovieMetaM documentation, Joomla
article). One section per help file, in the order of the help page; `#` → section title, `##` →
`h3`, `>` → note (`mm-story`), images → `figure` with caption (`:/images/help/` →
`images/software/pdfdocumentscannerm/help/`), bold text that is another topic's title → link to
it (not inside that topic itself). The version line of the about page is left out. Page addresses:
`<de|en>/pdfdocumentscannerm/documentation`.


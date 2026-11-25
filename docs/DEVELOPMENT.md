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
| Save as PDF (selected / all pages), rotate, clear | done |
| OCR when saving (tesseract, buttons "Save" / "Save with OCR" and language in the save dialog) | done |
| Fake duplex | open (button exists, not connected) |
| Straighten right after scanning (check box in the settings) | done |
| Crop by hand (button next to rotate, frame in the preview) | done |

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
| `qt6-base`, `qt6-svg` | 6.12.0 | Widgets, D-Bus (light / dark mode), SVG icons |
| `qt6-tools` | 6.12.0 | `lupdate` / `lrelease` for the `.ts` files |

**Runtime**

| For | Tested (Arch) | Arch/CachyOS | Debian/Ubuntu | Fedora/RHEL | openSUSE |
|---|---|---|---|---|---|
| `scanimage` (options, scanning) and the `escl` backend | 1.4.0 | `sane` | `sane-utils` | `sane-backends` | `sane-backends` |
| `airscan-discover`, the fast scanner search | 0.99.38 | `sane-airscan` | `sane-airscan` | `sane-airscan` | `sane-airscan` |
| mDNS (service `avahi-daemon` running), finds the network scanners | 0.9rc5 | `avahi` | `avahi-daemon` | `avahi` | `avahi` |
| OCR when saving (optional) | 5.5.4 | `tesseract` | `tesseract-ocr` | `tesseract` | `tesseract-ocr` |

Without `sane-airscan` the app falls back to `scanimage -L` (about 10 s, often misses devices).

**OCR language data:** one package for every OCR language; the save dialog offers what
`tesseract --list-langs` reports (without `osd`). Under Arch English is **not** included in
`tesseract` (it only pulls in `osd` and any one language package); Debian's `tesseract-ocr`
brings English along. Without tesseract or without a language "Save with OCR" is disabled.

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
| `main.cpp` | application and organization name, Fusion style, light / dark mode, INI format, translators, main window |
| `colorschemewatcher.*` | light / dark setting of the desktop from the XDG desktop portal, reports changes (copy from NiriSettingsM) |
| `updatechecker.*` | update check: latest release from the GitHub API (`releases/latest`, tag vs. `PROJECT_VERSION`), at most once a day; the button "Update" opens the release page (the AppImage name carries the version, so no fixed download link). From MovieMetaM |
| `mainwin.*`, `initialize.cpp` | main window: scanner search, options, scanning, button states; `initialize.cpp` builds toolbars, form, connections and restores the window layout |
| `settingsmanager.*` | singleton: all settings (INI) and the options of the current scanner |
| `settingsoverlay.*` | settings popup with the nose below the settings button |
| `statusoverlay.*` | messages over the window (search, loading, scanning, errors) |
| `pagelistwidget.*` | page list: one column, page size follows the width; select, remove, drag and drop |
| `pagedelegate.*` | draws a page: image, label "Page N", selection, × on hover |
| `sanedevice.*` | how scanimage opens a scanner: escl scanners through the airscan backend (section 4) |
| `ocr.*` | tesseract: installed languages and their names |
| `pdfsavedialog.*` | file dialog for saving, buttons "Save" / "Save with OCR" and OCR language |
| `pageprocessing.*` | processing of a scanned page: landscape turn, straighten, color mode, black/white threshold, rotating, content rectangle and cropping |
| `previewview.*` | large preview (`QGraphicsView`), fits the image into the view, zoom, crop frame |
| `helppage.*`, `twolinedelegate.*` | help page (Markdown in `help/<de\|en>/`, `Help.qrc`); copies from MovieMetaM, adapted to `UiColors` |
| `uicolors.h` | surface colors, derived from Window/WindowText only; the app palette (`applyStandardPalette`) |
| `toolbarm.*` | toolbar; copy from `../libraries/toolbarm` (master there, version 1.8.0) |
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

### Opening: escl scanners through airscan (`sanedevice.*`)

The escl backend of SANE 1.4 takes **only the first page of a feeder job**: the scanner pulls the
whole stack in one job, escl starts a new job for page 2 and gets "out of documents" (seen with the
HP and the Brother). The airscan backend takes every page of the job. So `SaneDevice::forDevice()`
opens escl scanners through airscan when `sane-airscan` is installed:

- Our own SANE configuration in `~/.cache/SpeckenbachNet/PDFDocumentScannerM/sane/`: `dll.conf`
  with only `airscan`, `airscan.conf` with the scanner entered by its eSCL URL and
  `discovery = disable`. Written before every `--help` and scan.
- scanimage runs with `SANE_CONFIG_DIR` pointing there (only our processes; the system
  configuration stays untouched) and `-d airscan:e0:Scanner` – always the same name, opening needs
  no search (0.04 s). airscan's own IDs (`airscan:e0:…`, `w3`) change with every search.
- The stored device ID stays the escl one. The eSCL URL comes from `airscan-discover`
  (`SettingsManager::scannerUrls`), for the saved scanner also from the INI
  (`Scanner/DeviceUrl`); otherwise `http://IP:port/eSCL/`.
- Without `sane-airscan`, and for other backends: as before.
- airscan reports `*unset*` as default of the scan intent; the parser uses "Document" instead
  (the combo could not show `*unset*`).

### Options

`getScannerCapabilities()` runs `scanimage -d <id> --help` and reads `--resolution`, `--mode`,
`--source`, `--scan-intent` (value list and default in `[...]`). Ranges like `50..1200` become
the common resolutions within (`expandResolutionValues`). Options marked `[inactive]` are
skipped. The largest scan area comes from `-x 0..215.9mm` / `-y 0..297.011mm`
(`scanAreaMaxWidth/Height`); it depends on the source (HP M277dw: flatbed 297 mm long, feeder
381 mm) and is stored for the default source only (`scanAreaSource`). A saved value the scanner does not offer is replaced by its default – except the color
mode: it is the app's own (next section), `--mode` only decides what to scan with.

---

## 5. Scanning and Pages

### Scanning (`MainWin::startScan()`)

- `scanimage` with `--device-name`, and only the options the scanner reported (otherwise
  `scanimage` aborts), `-x`/`-y` from the page format, `--format=png`.
- The scan area is always upright, even in landscape: the sheet lies upright on the glass or in
  the feeder (A4 landscape is wider than the glass). Each page is turned 90° counter-clockwise
  afterwards (`convertScannedPage(…, landscape)`).
- `-x`/`-y` are cut down to the largest scan area when scanning with `scanAreaSource`: airscan
  does that itself, escl passes too large values on unchecked.
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
- Rotating keeps the image format (`PageProcessing::rotatedImage()`): Qt turns a 1 bit
  image into 32 bit while rotating.
- Older settings with `Lineart` are read as `BlackWhite`.

### Straighten (`pageprocessing.*`)

Setting `Scanner/Straighten` (check box "Straighten Document", off by default), read when the
scan starts. Done in `convertScannedPage()` right after the page arrives: landscape turn →
straighten → color mode. Black/white pages are straightened in gray and thresholded afterwards
(a turned 1 bit image gets ragged letters).

- `skewAngle()`: the page scaled to 1000 px width (about 100 dpi), dark points by Otsu, outer
  3 % left out (edge shadows). For each angle every point is counted in the row
  `y − x · tan(angle)`; the angle whose row profile has the largest sum of squared neighbor
  differences wins (text lines fall on few rows). Search ±5°: coarse in 0.25° steps, then
  0.025° around the best.
- No turn if: less than 0.2 % or more than 35 % of the page is dark (blank page, photo), the
  best score is under 1.5 × the median of the coarse scores (no clear lines), or the angle is
  below 0.2°.
- `straightenedImage()`: painted in 32 bit with `SmoothPixmapTransform`, turned by −angle around
  the center, same size, uncovered corners white; back to the page's format, resolution kept.
- The angle comes back through `convertScannedPage(..., &straightenedBy)` and is stored in the
  item (`PageListWidget::kStraightenedRole`); the label reads "Page 3 · straightened 2.8°"
  (`setPageLabel()`, also on renumbering), full text as tooltip, the delegate elides it.
- Measured with synthetic A4 pages at 300 dpi (tilt 0.3° … 4.9°, both directions): angle found to
  0.025°, about 60 ms to find and 70 ms to turn. Not yet tested with real scans.


### Crop (`previewview.*`, `pageprocessing.*`)

By hand, one page at a time: the checkable toolbar button `cropBtn` (next to rotate) calls
`PreviewView::startCrop()` for the page in the preview. Enabled for exactly one selected page that
is also the current one; while the frame is shown the other page buttons are disabled.

- Start frame: `PageProcessing::contentRect()` – pixels darker than 160 (also the gray print of a
  till receipt), an edge strip of 3 mm left out (lid shadow), rows / columns with at least
  3 dark pixels; the box around them plus 4 mm, the whole page if nothing is found.
- `PreviewView` draws the frame in `drawForeground()` (scene = image pixels; outside darkened,
  frame and handles in the accent color, sizes in view pixels). Corners and edges resize
  (grip 8 px), inside moves, outside draws a new frame; minimum 16 image pixels, clamped to the
  page. While cropping the drag mode is `NoDrag`, zoom stays.
- Enter (shortcut only enabled while cropping) or "Apply" in the `CropBar` (lower left) emits
  `cropAccepted(path, rect)`; Esc (MainWin's shortcut, before scan / OCR cancel), "Cancel", the
  button again, a new page or an empty preview end it. A frame over the whole page cuts nothing.
- `PageListWidget::cropPage()` cuts the file (`croppedImage()`, format and resolution kept) and
  the gray original of a black/white page, like rotating; the PDF page gets the size of the cut
  from the pixels and the resolution.

### Page list (`PageListWidget`, `PageDelegate`)

- ListMode, one column. Pages fill the width (A4 ratio). If they do not fit the height, they
  shrink at most by the width of the scroll bar before the bar appears (`updateItemSizes`).
- Select: click, Ctrl/Shift + click, Ctrl+A. Remove: × on hover, Delete/Backspace, context
  menu "Remove n pages" – the image file stays until the app closes (undo). Labels are renumbered.
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
- Clear: all pages, after a question (undoable like removing).
- **Undo / redo** (`QUndoStack` in `PageListWidget`, Ctrl+Z / Ctrl+Shift+Z as window shortcuts of
  the list, entries in the context menu, also on the empty list): one `PageCommand` (two
  functions) per step. Pages are known by their file path.
  - Remove, clear, reorder: the order before and after; `applyOrder()` takes pages out into
    `m_takenOut` (kept, deleted with the list) or puts them back. Pages scanned meanwhile are in
    neither order and stay behind the restored ones. Removed image files stay until
    `MainWin::removeScanDirs()` at closing.
  - Rotate, crop: copies of the page image and its gray original before and after the change,
    `undo_<n>_b_page_001.png` / `undo_<n>_a_…` next to the page (not `page_*`: the scan watches
    for those); undo / redo copy them back and reload the icon.
  - After a step the affected pages are selected, the first one is shown; the status overlay
    says "Undone: Crop" (`undoMessage()`).
  - Not while scanning, saving with OCR (list disabled) or cropping (`setUndoAllowed()` from
    `MainWin::updateButtons()`). Adding a scanned page is no step.

### Saving as PDF (`MainWin::savePdf()`)

- Selected or all pages in list order (`PageListWidget::pages()`).
- File dialog: name `Scan_<yyyy-MM-dd_HHmm>.pdf`, suggested in the folder of the last save
  (INI `Save/LastFolder`; before the first save the documents folder).
- `writePdf()` uses `QPdfWriter` (Qt GUI, no extra dependency): one page per image, the image
  fills the page. The page size follows from pixels and scan resolution (`pageLayoutFor()`,
  `FuzzyMatch` turns 209.97 × 297.01 mm into A4); landscape pages get a landscape layout,
  because `QPageSize` alone is always portrait.
- The file dialog is Qt's own (`PdfSaveDialog`, `DontUseNativeDialog`): only that one takes extra
  buttons. Qt's button box (vertical, ordered by the desktop's rules) is hidden; own buttons in a
  fixed order: **Cancel | Save with OCR | Save** (default, Enter). They follow Qt's hidden save
  button (`EnabledChange`: disabled without a file name) and call `QFileDialog::accept()`
  (checks the name, asks before overwriting). Above them the OCR language, preset from the
  settings (`Scanner/OcrLanguage`); a change there is for this save only and not stored. Without
  a stored language the system's, else the first one (`Ocr::fillLanguageCombo`).
- Look: the file type row is hidden (only PDF), margins 20 px, file name / folder / language
  rounded, sidebar and file list as rounded boxes with 26 px rows (`UiColors::applyInputStyle`,
  `applyComboStyle`, `applyBoxStyle`, `listItemRules`, copied from NiriSettingsM's
  `themecolors.h`). Qt's parts are found by object name (`fileNameEdit`, `lookInCombo`,
  `sidebar`, `listView`, `treeView`); a missing one keeps Qt's look. Set again on a
  light / dark switch (the style sheets hold fixed colors). Size and view are remembered.
- **OCR happens when saving** (variant A): tesseract writes the PDF from the same images instead
  of `QPdfWriter` (`tesseract list.txt <target>.part -l deu pdf`, image and invisible text in one
  run). Page size from the resolution in the PNG (`pHYs`, written by scanimage, kept by Qt when
  converting and rotating). Measured: the PDF is not bigger than with `QPdfWriter`, about 4 s
  per A4 page at 300 dpi.
- tesseract puts a PNG into the PDF as it is (Flate), other files it encodes itself (Leptonica:
  1 bit → CCITT G4). So black/white pages go to it as uncompressed 1 bit TIFF
  (`PageProcessing::writeBlackWhiteTiff()`, own writer with the resolution tags – Qt's TIFF
  writer is a plugin and only knows LZW). Measured with two real text pages: 110 KB → 70 KB,
  same text, same picture. Smaller would be JBIG2 (PDFScanner on the Mac: 45 KB) – needs
  jbig2enc and an own PDF writer, later.
- `writeOcrPdf()` runs its own `QEventLoop`: the window stays responsive, the status message
  shows "page n of m" (from tesseract's lines `Page n : …`), **Esc** cancels. The page list is
  locked meanwhile (the images must not change), closing waits. tesseract writes
  `<target>.part.pdf`, renamed only at the end: a cancel or error leaves an existing file as it is.
- Straightening happens right after scanning (see Straighten), cropping by hand on the image file;
  saving then only does OCR → PDF. Rejected for now: OCR right after scanning with
  selectable text in the preview (variant B) – rotating, straightening and cropping would have to
  work on recognized text.
- Without OCR writing blocks the window; a `processEvents()` lets the message "Saving PDF..."
  appear first.

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
All access goes through `SettingsManager`, except the update keys (`UpdateChecker`, as in
MovieMetaM).

| Key | Content |
|---|---|
| `Scanner/DeviceId`, `Scanner/DeviceName`, `Scanner/DeviceUrl` | the user's scanner (only written on the user's choice) |
| `Scanner/ScanningUnit`, `Resolution`, `ScanIntent` | scanner options |
| `Scanner/ColorMode` | `Color`, `Gray` or `BlackWhite` (the app's modes) |
| `Scanner/PageOrientation`, `PageFormat` | page |
| `Scanner/OcrLanguage` | preset of the OCR language in the save dialog, tesseract code (`deu`, `eng`) |
| `Save/DialogGeometry`, `Save/DialogState` | size and view of the save dialog (`saveGeometry()` / `QFileDialog::saveState()`, written in `done()`) |
| `Window/Geometry`, `Window/Splitter` | `saveGeometry()` / `QSplitter::saveState()`, written in `closeEvent` |
| `Save/LastFolder` | folder of the last saved PDF |
| `Scanner/Straighten` | straighten new scans (check box "Straighten Document") |
| `General/CheckUpdates` | ask GitHub for a new version (default on) |
| `General/LastUpdateCheck`, `General/LatestVersion` | day of the last request and the version found: at most one request a day, a newer version is shown again on the next start without a request |

First start without saved window: 900 × 600 (`kDefaultSize` in `restoreWindowLayout()`, fits
laptops with 150 % scaling), page list 180 px. Qt widens the window by itself if the toolbar
needs more room. Under Wayland the window position is not restored (the compositor decides).

---

## 7. Translations

- `i18n/PDFDocumentScannerM_de.ts`, `_en.ts`; `qt_add_translations` builds the `.qm` files into
  `i18n/`, `translation.qrc` embeds them.
- Many sources are keys (`SCAN`, `REMOVE_PAGES %n`), so English needs a translation too. Newer
  texts are plain English sentences.
- `main.cpp` loads the first system language with a translation, otherwise English, and Qt's own
  `qtbase` translation in the same language (standard buttons, file dialog).
- Help texts: `help/de/*.md`, `help/en/*.md` (`Help.qrc`).

---

## 8. UI Building Blocks and Style

- **Light / dark mode:** style Fusion. `ColorSchemeWatcher` reads `color-scheme` from the XDG
  desktop portal and reports changes; `main.cpp` sets it with `QStyleHints::setColorScheme()` and
  then `UiColors::applyStandardPalette()`: always Fusion's own light or dark palette (a platform
  theme such as gtk3 has colors for one mode only, mixed with Fusion's they looked different after
  switching back and forth), with a readable selection color. Works the same with any
  `QT_QPA_PLATFORMTHEME` and in the AppImage.
- **Colors are derived from Window/WindowText only** (`uicolors.h`, `UiColors::mix`). Originally
  because with the gtk3 palette Window, Button and Base are equal and Light/Mid/Dark nearly so;
  with the Fusion palette still the safe way.
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

Light / dark mode in the AppImage: a theme set in the environment (e.g.
`QT_QPA_PLATFORMTHEME=gtk3` under niri) is missing there. That no longer matters: the app asks
the desktop portal itself (`ColorSchemeWatcher`, section 8) and uses Fusion's palette. Qt's own
translations (`qtbase_*.qm`) are put into the package by linuxdeploy (`usr/translations`).

- The name of the package carries the version (`PDFDocumentScannerM-0.5.1-linux.AppImage`); the
  script reads it from `CMakeLists.txt`.
- `main.cpp` sets the desktop file name (`net.speckenbach.PDFDocumentScannerM`): under Wayland it
  ties the window to the start menu entry and its icon.
- The update check needs HTTPS: Qt's TLS plugin (`plugins/tls`, OpenSSL backend) must be in the
  AppImage. Check after the first build with it (0.8.0) that the button appears in the AppImage
  too (e.g. with an older `General/LatestVersion` removed and the date reset).
- `scanimage`, `airscan-discover` and `tesseract` are **not** in the package; they come
  from the system (see Prerequisites). Not tested yet: whether the app started from the AppImage
  runs them without problems.

### Documentation for the website (`tools/help-to-html.py`)

Turns the help into the documentation pages (same HTML as the MovieMetaM documentation, Joomla
article). One section per help file, in the order of the help page; `#` → section title, `##` →
`h3`, `>` → note (`mm-story`), images → `figure` with caption (`:/images/help/` →
`images/software/pdfdocumentscannerm/help/`), bold text that is another topic's title → link to
it (not inside that topic itself). The version line of the about page is left out. Page addresses:
`en/pdfdocumentscannerm/documentation` and `de/pdfdocumentscannerm/dokumentation`
(`alias` in `TEXTS`; the language button links to the alias of the other language).


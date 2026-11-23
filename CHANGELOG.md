# PDFDocumentScannerM – Changelog

## 0.5.4 – Ask before closing, landscape, dark mode (November 23, 2026)

- Scanner options load in well under a second instead of about 10 seconds (measured with the HP M277dw)
- Closing asks first if pages are not saved yet (save all, discard, cancel) and stops a running scan
- The scanned images are deleted from the temporary folder when the app closes
- The empty preview shows which scanner is used (or why there is none); tooltip on the scan button
- Help: the text stands in the middle of the page
- Page list and preview are gray instead of almost white in light mode
- The entries of "Scan purpose" are translated
- Light / dark mode follows the desktop at once (XDG desktop portal, also in the AppImage), with Fusion's own light and dark colors
- Qt's own texts (standard buttons, file dialog) in the language of the app
- Landscape no longer cuts off A4, Letter and Legal pages: the sheet is scanned upright and turned afterwards
- The scan area is limited to what the scanner can scan (e.g. Legal on an A4 flatbed)

## 0.5.3 – All pages from the feeder (November 14, 2026)

- The feeder now delivers every page (scanning goes through sane-airscan when it is installed)
- Fixed: the language button of the online documentation linked to the wrong German page
- Fixed: the Linux package stayed light in dark mode under some desktops (e.g. niri)
- Fixed: scroll bars stayed in the empty preview after clearing the pages

## 0.5.2 – Help and Linux package (November 14, 2026)

- Built-in help for scanning, pages, settings and saving, in English and German
- First Linux package (AppImage); the window shows the app icon in the task bar
- "Your language is missing?" – help translating the app
- Known issue: with the feeder only the first page arrives on some scanners (HP M277dw)

## 0.5.1 – Cancel a scan (October 27, 2026)

- Cancel a running scan with the scan button (it reads "Cancel" while scanning) or Esc; pages already scanned are kept
- Known issue: with the feeder only the first page arrives on some scanners (HP M277dw)

## 0.5 – Color modes (October 26, 2026)

- Color, grayscale and black/white for every scanner; black/white is made by the app with an automatic threshold (about 60 KB per page in the PDF)

## 0.4 – Saving as PDF (October 26, 2026)

- Save the selected or all pages as PDF; the page size follows the scan
- Rotate pages, clear all pages

## 0.3 – Pages, help and a faster search (October 26, 2026)

- Page list: select, remove (×, Delete, context menu), reorder by drag and drop
- Zoom in the preview (Ctrl + mouse wheel, Ctrl + Plus / Minus / 0, double click, zoom bar)
- Fast scanner search; the chosen scanner is kept even if a search misses it
- Window size and page list width are remembered
- Help page, coffee button

## 0.2 – First usable version (October 26, 2026)

- Scan from network scanners (eSCL) with flatbed and feeder
- Settings: source, resolution, page format, orientation
- Fits light and dark themes, also gtk3
- English and German

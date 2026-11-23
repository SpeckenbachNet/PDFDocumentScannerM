# PDFDocumentScannerM

Scan documents with your network scanner and save them as PDF – on Linux.

## What's new in 0.5.4

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

All changes: [CHANGELOG.md](CHANGELOG.md)

## Features

- Finds network scanners (eSCL / AirScan) in a few seconds and remembers your scanner
- Flatbed and document feeder
- Color, grayscale and black/white – black/white is made by the app for every scanner, so text
  pages stay small (about 60 KB per page in the PDF)
- Pages appear while scanning; select, reorder by drag and drop, rotate, remove
- Large preview with zoom (Ctrl + mouse wheel, Ctrl + Plus / Minus / 0, double click)
- Save the selected or all pages as one PDF
- English and German, light and dark themes

## Requirements

The app uses the scanner tools of your system:

| For | Arch / CachyOS | Debian / Ubuntu | Fedora / RHEL | openSUSE |
|---|---|---|---|---|
| Scanning (`scanimage`) | `sane` | `sane-utils` | `sane-backends` | `sane-backends` |
| Fast scanner search, all pages from the feeder | `sane-airscan` | `sane-airscan` | `sane-airscan` | `sane-airscan` |
| Finding network scanners (service running) | `avahi` | `avahi-daemon` | `avahi` | `avahi` |

Tested on CachyOS; the package names of the other distributions are not tested.

## Your scanner

- It must offer **eSCL** (AirScan) in the network and have an **IPv4 address**.
- Give it a **fixed IP address** (DHCP reservation in your router): the app remembers the scanner
  by its address.
- **HP Color LaserJet MFP M277dw:** switch off the redirect from HTTP to HTTPS in the printer's web
  interface.
- Tested with HP Color LaserJet MFP M277dw and Brother MFC-L2710DW.

## Installation

1. Download `PDFDocumentScannerM-<version>-linux.AppImage` from the release.
2. Make it executable: `chmod +x PDFDocumentScannerM-*-linux.AppImage`
3. Start it.

## Your language is missing?

Help translate PDFDocumentScannerM: https://software.speckenbach.net/en/pdfdocumentscannerm/translation

## License

GNU General Public License version 3 – see [LICENSE](LICENSE).

Developer documentation: [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)

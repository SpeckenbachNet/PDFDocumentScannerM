# PDFDocumentScannerM

Scan documents with your network scanner and save them as PDF – on Linux.

## What's new in 0.5.1

- Cancel a running scan with the scan button (it reads "Cancel" while scanning) or Esc; pages
  already scanned are kept
- Known issue: with the feeder only the first page arrives on some scanners (HP M277dw)

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
| Fast scanner search | `sane-airscan` | `sane-airscan` | `sane-airscan` | `sane-airscan` |
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

## License

GNU General Public License version 3 – see [LICENSE](LICENSE).

Developer documentation: [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)

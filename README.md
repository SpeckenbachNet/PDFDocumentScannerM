# PDFDocumentScannerM

Scan documents with your network scanner and save them as PDF – on Linux.

## What's new in 0.6.0

- Text recognition (OCR) when saving: the save dialog has the buttons "Save" and "Save with OCR", the PDF becomes searchable and text can be copied (needs tesseract)
- The OCR language is preset in the settings and can be changed in the save dialog for one save
- The languages offered are the ones installed for tesseract
- Saving with text recognition shows the page it is at and can be cancelled with Esc
- Straighten and crop are grayed out in the settings until they work
- Rounded combo boxes and more room in the settings
- Save dialog: rounded fields and lists, more room, no file type row; it remembers its size and view

All changes: [CHANGELOG.md](CHANGELOG.md)

## Features

- Finds network scanners (eSCL / AirScan) in a few seconds and remembers your scanner
- Flatbed and document feeder
- A4, A5, A6, Letter and Legal, portrait or landscape
- Color, grayscale and black/white – black/white is made by the app for every scanner, so text
  pages stay small (about 60 KB per page in the PDF)
- Pages appear while scanning; select, reorder by drag and drop, rotate, remove
- Large preview with zoom (Ctrl + mouse wheel, Ctrl + Plus / Minus / 0, double click)
- Save the selected or all pages as one PDF
- Text recognition (OCR) with tesseract when saving: searchable PDFs
- English and German, light and dark themes

## Requirements

The app uses the scanner tools of your system:

| For | Arch / CachyOS | Debian / Ubuntu | Fedora / RHEL | openSUSE |
|---|---|---|---|---|
| Scanning (`scanimage`) | `sane` | `sane-utils` | `sane-backends` | `sane-backends` |
| Fast scanner search, all pages from the feeder | `sane-airscan` | `sane-airscan` | `sane-airscan` | `sane-airscan` |
| Finding network scanners (service running) | `avahi` | `avahi-daemon` | `avahi` | `avahi` |
| Text recognition (optional) | `tesseract` | `tesseract-ocr` | `tesseract` | `tesseract-ocr` |
| OCR language, e.g. German | `tesseract-data-deu` | `tesseract-ocr-deu` | `tesseract-langpack-deu` | `tesseract-ocr-traineddata-german` |

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

# Scanning

PDFDocumentScannerM scans with scanners in your network. It finds them by itself and remembers the one you chose.

## Finding the scanner

At the start PDFDocumentScannerM searches the network for scanners; this takes a few seconds. Then it loads the options of your scanner (resolutions, color modes, flatbed or feeder). As soon as **Scan** is active, you can start.

- Choose the scanner in the **Settings** (the gear in the toolbar). PDFDocumentScannerM remembers it.
- The button with the two arrows searches again, for example after you switched the scanner on.

If a search misses your scanner, PDFDocumentScannerM asks it directly. Only if it does not answer either, another scanner is used – for this session only. The next start tries your scanner again.

## Scanning

1. Put the document on the glass or into the feeder.
2. Choose **Flatbed** or **ADF** (feeder) under **Scanning unit** in the **Settings**. **Automatic** leaves the choice to the scanner.
3. Click **Scan**.

The flatbed scans one page, the feeder all pages it holds. Each page appears in the page list as soon as it is scanned; the newest page is shown large on the right. More about the pages under **Pages**.

## Cancelling

While scanning, **Scan** turns into **Cancel**. A click on it or the Escape key stops the scan. Pages that are complete already stay in the list.

## What your scanner needs

- It must offer **eSCL** (also called AirScan) in the network – most network scanners and multifunction printers of the last years do.
- It needs an **IPv4 address**. Best give it a fixed address in your router: PDFDocumentScannerM remembers the scanner by its address.
- **HP Color LaserJet MFP M277dw:** switch off the redirect from HTTP to HTTPS in the printer's web interface, otherwise scanning fails.

On your computer the scanner tools must be installed: `sane` (on Debian and Ubuntu `sane-utils`) and `sane-airscan`.

# Settings

The gear in the toolbar opens the settings below it. Every change applies at once and is remembered. A click beside the settings closes them.

## Scanner

The scanner to scan with. After a change PDFDocumentScannerM loads its options; the settings close meanwhile. If no scanner was found, the list says so and the scanner options are locked.

## Scanning unit

**Flatbed** (glass) or **ADF** (document feeder) – the names come from the scanner. **Automatic** leaves the choice to the scanner.

## Color mode

- **Color** – as the scanner delivers it.
- **Grayscale** – for photos and documents with gray areas.
- **Black/white** – for text. PDFDocumentScannerM scans in gray and makes each page black and white itself, with a threshold it finds for every page. The PDF stays small: about 60 KB per text page.

These three modes work with every scanner, also with one that cannot scan in gray or black and white itself.

## Scan intent

What the scan is for, for example **Document** or **Photo**; the scanner adjusts its processing. The list only appears if the scanner offers it.

## Resolution

Dots per inch (dpi). 300 dpi is good for text, higher values for small print and photos – they take longer and make bigger files.

## Page orientation and page format

**Portrait** or **Landscape**, and the size of the scanned area: A4, A5, A6, Letter or Legal. **Standard** takes A4, or Letter in countries that use it.

The sheet always lies upright on the glass or in the feeder. With **Landscape** the app turns each scanned page 90° counter-clockwise afterwards. If it ends up upside down, turn it with the button with the round arrow. If the format is larger than the scanner can scan (for example Legal on an A4 glass), only the area it can handle is scanned.

## OCR language

The language for text recognition when saving (**Save with OCR**, see **Saving**). The save dialog starts with it. The languages installed for tesseract can be chosen; **Not installed** means tesseract or a language is missing.

## Straighten Document

If a sheet lies slightly askew on the glass or runs through the feeder at a slant (up to 5°), PDFDocumentScannerM turns the page straight right after scanning. The app goes by the lines of text; pages without text, such as photos, and blank pages stay as they are. The uncovered corners become white. The page list then shows below the page how much it was straightened, for example **Page 3 · straightened 2.8°**. The setting applies to the next scans.

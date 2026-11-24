# Saving

The scanned pages become one PDF file.

## Saving a PDF

- **Save selected** saves the selected pages.
- **Save all** saves all pages.

The pages come in the order of the page list – reorder them first if needed (see **Pages**). A file dialog asks where to save; the suggested name has the date and time, for example `Scan_2026-11-14_0930.pdf`. Next time the dialog opens in the folder you saved in last.

Each page of the PDF has the size that was scanned, so an A4 scan becomes an A4 page and a rotated page a landscape page.

## Text recognition (OCR)

At the bottom of the save dialog a button decides:

- **Save** – the PDF holds only the images of the pages (also with the Enter key).
- **Save with OCR** – PDFDocumentScannerM recognizes the text of the pages and lays it invisibly over the images: in the PDF you can search, select and copy text. The pages look exactly as without OCR.

Above them is the **OCR language**. It starts with the language from the **Settings**; a change in the dialog is for this one save only.

- Text recognition takes a few seconds per page. The message shows which page it is at; **Esc** cancels, then no PDF is written.
- The languages installed for tesseract can be chosen. If **Save with OCR** is grayed out, tesseract or a language is missing – on Arch for example the package `tesseract-data-eng`, on Debian and Ubuntu `tesseract-ocr-eng`.

## File size

Black and white pages are small (about 60 KB per text page), grayscale and color pages much bigger. For text, **Black/white** in the **Settings** is the best choice.

> The page list is not kept when you close PDFDocumentScannerM, the scanned images are deleted. If some pages are not saved yet, PDFDocumentScannerM asks first: **Save all**, **Discard** or **Cancel**.

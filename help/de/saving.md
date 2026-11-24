# Speichern

Aus den gescannten Seiten wird eine PDF-Datei.

## Eine PDF speichern

- **Auswahl speichern** speichert die ausgewählten Seiten.
- **Alles speichern** speichert alle Seiten.

Die Seiten kommen in der Reihenfolge der Seitenliste – sortiere sie vorher um, wenn nötig (siehe **Seiten**). Ein Dateidialog fragt, wohin; der vorgeschlagene Name enthält Datum und Uhrzeit, zum Beispiel `Scan_2026-11-14_0930.pdf`. Beim nächsten Mal öffnet sich der Dialog in dem Ordner, in dem du zuletzt gespeichert hast.

Jede Seite der PDF hat die gescannte Größe: Aus einem A4-Scan wird eine A4-Seite, aus einer gedrehten Seite eine Seite im Querformat.

## Texterkennung (OCR)

Unten im Speicherdialog entscheidest du per Knopf:

- **Speichern** – die PDF enthält nur die Bilder der Seiten (auch mit der Eingabetaste).
- **Mit OCR speichern** – PDFDocumentScannerM erkennt den Text der Seiten und legt ihn unsichtbar über die Bilder: In der PDF kannst du dann suchen und Text markieren und kopieren. Die Seiten sehen genauso aus wie ohne OCR.

Darüber steht die **OCR-Sprache**. Sie ist auf die Sprache aus den **Einstellungen** gestellt; änderst du sie im Dialog, gilt das nur für dieses eine Speichern.

- Die Texterkennung dauert einige Sekunden pro Seite. Die Meldung zeigt, bei welcher Seite sie ist; **Esc** bricht ab, dann wird keine PDF geschrieben.
- Zur Auswahl stehen die Sprachen, die für tesseract installiert sind. Ist **Mit OCR speichern** ausgegraut, fehlt tesseract oder eine Sprache – unter Arch zum Beispiel das Paket `tesseract-data-deu`, unter Debian und Ubuntu `tesseract-ocr-deu`.

## Dateigröße

Schwarz-weiße Seiten sind klein (etwa 60 KB pro Textseite), Graustufen- und Farbseiten deutlich größer. Für Text ist **Schwarz/Weiß** in den **Einstellungen** die beste Wahl.

> Die Seitenliste bleibt beim Schließen von PDFDocumentScannerM nicht erhalten, die gescannten Bilder werden gelöscht. Sind noch Seiten nicht gespeichert, fragt PDFDocumentScannerM vorher nach: **Alles speichern**, **Verwerfen** oder **Abbrechen**.

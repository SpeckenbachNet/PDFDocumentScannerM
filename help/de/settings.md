# Einstellungen

Das Zahnrad in der Werkzeugleiste öffnet darunter die Einstellungen. Jede Änderung gilt sofort und wird gemerkt. Ein Klick neben die Einstellungen schließt sie.

## Scanner

Der Scanner, mit dem gescannt wird. Nach einem Wechsel lädt PDFDocumentScannerM seine Möglichkeiten; die Einstellungen schließen sich so lange. Wurde kein Scanner gefunden, steht das in der Liste, und die Scanner-Einstellungen sind gesperrt.

## Scannereinheit

**Flatbed** (Glas) oder **ADF** (Einzug) – die Namen kommen vom Scanner. **Automatisch** überlässt die Wahl dem Scanner.

## Farbmodus

- **Farbe** – so, wie der Scanner es liefert.
- **Graustufen** – für Fotos und Dokumente mit grauen Flächen.
- **Schwarz/Weiß** – für Text. PDFDocumentScannerM scannt in Grau und macht jede Seite selbst schwarz-weiß, mit einem Schwellwert, den es für jede Seite eigens findet. Die PDF bleibt klein: etwa 60 KB pro Textseite.

Diese drei Modi gibt es bei jedem Scanner, auch bei einem, der selbst nicht in Grau oder Schwarz-Weiß scannen kann.

## Scan-Zweck

Wofür gescannt wird, zum Beispiel **Document** oder **Photo**; der Scanner passt seine Bearbeitung daran an. Die Liste erscheint nur, wenn der Scanner das anbietet.

## Auflösung

Punkte pro Zoll (dpi). 300 dpi reichen für Text, höhere Werte für Kleingedrucktes und Fotos – sie dauern länger und ergeben größere Dateien.

## Seitenausrichtung und Seitenformat

**Hochformat** oder **Querformat** und die Größe des gescannten Bereichs: A4, A5, A6, Letter oder Legal. **Standard** nimmt A4, in Ländern mit dem Letter-Format Letter.

Das Blatt liegt immer hochkant auf dem Glas oder im Einzug. Bei **Querformat** dreht das Programm jede gescannte Seite danach um 90° gegen den Uhrzeigersinn. Liegt sie falsch herum, drehe sie mit dem Knopf mit dem runden Pfeil. Ist das Format größer, als der Scanner scannen kann (etwa Legal auf einem A4-Glas), wird nur der Bereich gescannt, den er schafft.

## OCR-Sprache

Die Sprache für die Texterkennung beim Speichern (**Mit OCR speichern**, siehe **Speichern**). Der Speicherdialog startet mit ihr. Zur Auswahl stehen die Sprachen, die für tesseract installiert sind; steht dort **Nicht installiert**, fehlt tesseract oder eine Sprache.

## Dokument ausrichten

Liegt ein Blatt leicht schief auf dem Glas oder läuft schräg durch den Einzug (bis 5°), dreht PDFDocumentScannerM die Seite gleich nach dem Scannen gerade. Das Programm richtet sich nach den Textzeilen; Seiten ohne Text, etwa Fotos, und leere Seiten bleiben, wie sie sind. Die frei werdenden Ecken werden weiß. In der Seitenliste steht dann unter der Seite, um wie viel sie ausgerichtet wurde, zum Beispiel **Seite 3 · ausgerichtet 2,8°**. Die Einstellung gilt für die nächsten Scans.

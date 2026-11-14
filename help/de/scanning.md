# Scannen

PDFDocumentScannerM scannt mit Scannern in deinem Netzwerk. Es findet sie selbst und merkt sich den, den du gewählt hast.

## Den Scanner finden

Beim Start sucht PDFDocumentScannerM im Netzwerk nach Scannern; das dauert ein paar Sekunden. Dann lädt es die Möglichkeiten deines Scanners (Auflösungen, Farbmodi, Flachbett oder Einzug). Sobald **Scannen** aktiv ist, kann es losgehen.

- Den Scanner wählst du in den **Einstellungen** (das Zahnrad in der Werkzeugleiste). PDFDocumentScannerM merkt ihn sich.
- Der Knopf mit den zwei Pfeilen sucht neu, zum Beispiel nachdem du den Scanner eingeschaltet hast.

Findet eine Suche deinen Scanner nicht, fragt PDFDocumentScannerM ihn direkt. Erst wenn er auch dann nicht antwortet, wird ein anderer Scanner benutzt – nur für diese Sitzung. Beim nächsten Start wird es wieder mit deinem versucht.

## Scannen

1. Lege das Dokument auf das Glas oder in den Einzug.
2. Wähle in den **Einstellungen** unter **Scannereinheit** **Flatbed** (Flachbett) oder **ADF** (Einzug). **Automatisch** überlässt die Wahl dem Scanner.
3. Klicke auf **Scannen**.

Das Flachbett scannt eine Seite, der Einzug alle Seiten, die darin liegen. Jede Seite erscheint in der Seitenliste, sobald sie gescannt ist; die neueste wird rechts groß gezeigt. Mehr zu den Seiten unter **Seiten**.

> Bekanntes Problem: Mit dem Einzug kommt bei manchen Scannern nur die erste Seite an (beobachtet beim HP Color LaserJet MFP M277dw).

## Abbrechen

Während des Scans wird aus **Scannen** **Abbrechen**. Ein Klick darauf oder die Esc-Taste hält den Scan an. Seiten, die schon fertig sind, bleiben in der Liste.

## Was dein Scanner braucht

- Er muss im Netzwerk **eSCL** anbieten (auch AirScan genannt) – das können die meisten Netzwerkscanner und Multifunktionsdrucker der letzten Jahre.
- Er braucht eine **IPv4-Adresse**. Gib ihm am besten in deinem Router eine feste Adresse: PDFDocumentScannerM erkennt den Scanner an seiner Adresse wieder.
- **HP Color LaserJet MFP M277dw:** Schalte in der Weboberfläche des Druckers die Umleitung von HTTP auf HTTPS aus, sonst scheitert das Scannen.

Auf deinem Rechner müssen die Scanner-Werkzeuge installiert sein: `sane` (unter Debian und Ubuntu `sane-utils`) und `sane-airscan`.

ScreenSaver v0.3.2 · Build 7 · 2026-11-26 · Phase 2 (Pre-Alpha) – Konzept

Status: Entwurf zur Besprechung. Nichts davon ist freigegeben. Implementierung erst nach Freigabe durch Housi.
Legende: [Housi] = von ihm entschieden/gesagt · [Vorlage] = aus RoutedMidi/Carbon gelesen · [Regelwerk] = aus Rules · [Vorschlag] = neu, zur Entscheidung · [Offen] = ungeklärt

## 1. Idee
- [Housi] Bildschirmschoner für Windows 11 in Form einer LED-Anzeige mit Farbverläufen und weiterem, Vorlage RoutedMidi (23.11.2026).
- [Housi] Aussehen: Carbon-Gerät mit Glasdisplay.
- [Housi] Was angezeigt wird, soll über eine Option immer wieder umstellbar sein.
- [Housi] Projektordner: C:\Users\hansg\Desktop\Claude\Bildschirmschoner (kein Plugin, daher nicht Claude Plugins).
- [Housi] Name: „ScreenSaver“ (26.11.2026, vorher „Bildschirmschoner LED“). Projektordner bleibt Desktop\Claude\Bildschirmschoner.
- [Housi] Entscheide 26.11.2026 (Abend): Schrift-Lizenz egal, das Programm wird nicht veröffentlicht; LED-Auflösung 16 × 64 passt vorerst; Phase 1 freigegeben.
- [Housi] Entscheide 26.11.2026: keine feste Akzentfarbe; Monitore wählbar je nach Hardware (zum Testen ein Monitor); Tipps-Fenster entfällt.

## 2. Technik (Machbarkeit)
- [Vorschlag] Ein Windows-Bildschirmschoner ist eine .scr-Datei (umbenannte EXE). Aufrufe: /s = Vollbild starten, /c = Einstellungen, /p <HWND> = Vorschau im Windows-Dialog. Ohne Parameter wie /c behandeln.
- [Vorschlag] Umsetzung nativ in C++ (Win32 + Direct2D/DirectWrite), ohne JUCE. Grund: JUCE ist für Audio-Plugins gedacht, für einen Bildschirmschoner (Vollbild auf jedem Monitor, Vorschau-Fenster, Beenden bei Maus/Taste) ein Umweg; ausserdem entfällt die JUCE-Lizenzfrage (Lizenzcheck.md). Keine Zusatzsoftware für Tester nötig.
- [Housi] Monitore: in den Einstellungen wählbar je nach Hardware; zum Testen genügt ein Monitor. [Vorschlag] Umsetzung: Liste der erkannten Monitore mit Häkchen, Standard nur der Hauptmonitor, nicht gewählte Monitore bleiben schwarz.
- [Vorschlag] Beenden bei Tastendruck, Mausklick oder grösserer Mausbewegung (kleine Schwellen gegen Zittern), wie Windows es erwartet.
- [Vorschlag] Einstellungen speichern unter %APPDATA% (Benutzerdaten bleiben bei Updates erhalten).
- [Vorschlag] Installer mit Inno Setup wie bei RoutedMidi, gleiche AppId bei allen Versionen. Installation kopiert die .scr in den Windows-Ordner, den die Bildschirmschoner-Liste liest (nach meinem Wissen System32, braucht Adminrechte) [Offen: in Phase 1 auf Housis Rechner prüfen].
- [Vorschlag] Update-Prüfung nach Regelwerk (GitHub housi666/<Produkt>, nur auf Knopfdruck oder einmal beim Öffnen der Einstellungen). Im Schonermodus (/s) nie Netzwerk, nie Download.
- [Vorschlag] Rendering per Direct2D, 30 fps genügt, GPU-schonend; bei Akkubetrieb optional 15 fps.

## 3. Aussehen: Carbon-Gerät mit Glasdisplay
- [Vorlage] Gehäuse-Verlauf #35383e → #1a1c1f, Bezel #050608, Bezel-Kante #474b52, Glas-Verlauf #0e1522 → #070a10 mit Glanzschicht (weiss ~18 %), Text #eef1f6, gedämpft #9aa3b2, Silk #b8bdc6, Muted #6b6e7c, LED aus #15171b / #25282e.
- [Vorlage] Schrift: Inter für UI; Display-Schrift Race Pod Narrow Slight-Italic wie RoutedMidi [Housi: Lizenz egal, nicht veröffentlicht; Phase 1 zeichnet die Ziffern ohnehin als eigene 5×7-LED-Schrift].
- [Vorlage] Glänzende Display-Knöpfe, Klick-Animation und farbige Knöpfe nach Regelwerk (in der Einstellungsoberfläche).
- [Vorschlag] Das Gerät steht mittig auf schwarzem Grund (ca. 65–80 % der Bildschirmbreite, Seitenverhältnis fest) und wandert sehr langsam (Pixel-Shift) gegen Einbrennen, wichtig bei OLED.
- [Vorschlag] LED-Feld als Punktmatrix: LEDs mit weichem Leuchten (Glow) und leichtem Gummitasten-Look wie in RoutedMidi; Auflösung Zeilen × Spalten einstellbar, Standard grob (z. B. 16 × 64) [Offen].
- [Housi] Keine feste Akzentfarbe (26.11.2026): die Farbverläufe sind das Thema.

## 4. Anzeige-Modi (umstellbar per Option)
- [Housi] Der Inhalt ist in den Einstellungen wählbar und jederzeit änderbar.
- [Vorschlag] Modi, in Stufen:
  1. Uhr & Datum (Punktmatrix-Ziffern), mit wanderndem Farbverlauf
  2. Laufschrift mit eigenem Text (z. B. Bandname, Spruch)
  3. Farbverlauf-Flächen / Plasma über die ganze LED-Matrix
  4. Wellen, Equalizer-Balken (Animation ohne Ton)
  5. Später: Reaktion auf den PC-Ton (Audio-Loopback) [Zukunft]
- [Vorschlag] Option „Modi abwechseln“ mit Dauer je Modus (Rotation) oder fester Modus.
- [Vorschlag] Farbverläufe: Auswahl aus Voreinstellungen (Regenbogen, Feuer, Eis, Metal-Rot/Magenta usw.) und zwei eigene Farben; Geschwindigkeit und Richtung einstellbar.

## 5. Einstellungsfenster (/c)
- [Vorschlag] Eigenes Fenster im Carbon-Stil (nicht der Standard-Windows-Dialog), mit Live-Vorschau des Geräts. Aufbau: Anzeige-Modus, Farbverlauf, Geschwindigkeit, Uhr (24 h / mit Sekunden), Text, Monitore, Pixel-Shift, Update.
- [Regelwerk] Nach Rules: Support-Knopf (Instagram/TikTok housi_666), ohne Tipps; Nutzungsbedingungen und Datenschutz, Changelog-Menü, Phasen-Abzeichen, Update-Funktion (Grundaufbau). Anzeige „Made by Housi“ und „Coded with Claude“ mit Produktname und Version.
- [Housi] Kein Tipps-Fenster (26.11.2026); Abweichung vom Regelwerk (Tipps-Seite), begründet durch den begrenzten Platz im Einstellungsfenster.
- [Vorschlag] Zoom per Mausrad ist für ein Vollbild-Gerät nicht sinnvoll; stattdessen Option „Gerätegrösse“.

## 6. Risiken und offene Punkte
- Lizenz der Display-Schrift: erledigt (nicht veröffentlicht, 26.11.2026). Gilt nur, solange das Programm nicht veröffentlicht wird.
- Installation nach System32 braucht Adminrechte; Verhalten auf Windows 11 prüfen.
- Mehrere Monitore mit unterschiedlicher Auflösung/Skalierung (DPI) sauber testen.
- Zeichenregeln und Sprache: Deutsch zuerst, Englisch später (Regelwerk).

## 7. Stand (v0.3.1, Build 6)
- Phase 1 gebaut (Build erfolgreich 26.11.2026): .scr mit /s, /c, /p; Carbon-Gerät mit Glasdisplay; LED-Matrix 16 × 64 mit Glow; Uhr HH:MM mit wanderndem Farbverlauf; Sekundenleiste; fünf Farbverläufe; Tempo; Pixel-Shift; Monitor-Auswahl.
- Neu in v0.3.0 (Code geschrieben, noch nicht kompiliert und getestet):
  - Einstellungsfenster nach Regelwerk im Carbon-Stil: glänzende Display-Knöpfe in eigenen Farben (Gruppen: Verlauf Magenta, Tempo Blau, Grösse Indigo, Sekundenleiste Petrol, Verschieben Violett, Monitore Grün, Support Wein, Update Neonpink, Phase Gelb), gemeinsamer Lichtstreifen, Klick-Animation (Rand und Text in Knopffarbe, Text 1 px nach unten, ausgelöst beim Loslassen), bei „weniger Bewegung“ ohne Streifen.
  - Live-Vorschau: ein Bildschirm im Seitenverhältnis des Hauptmonitors, darin das Gerät in der gewählten Grösse.
  - Gerätegrösse: 30 bis 100 Prozent, Schritt 5, Standard 80. Pixel-Shift bewegt das Gerät nur im freien Rand.
  - Support-Fenster („Support & Social Media“), Rechtstexte-Fenster mit zwei Reitern (Fassung 26.11.2026), Changelog-Fenster (Hauptversion, Nebenversion, Version, aufklappbar, Typen neu/besser/fix/intern), Phasen-Abzeichen mit Hinweis.
  - Update nach Regelwerk: GitHub housi666/ScreenSaver (bestätigt 26.11.2026), Prüfen nur auf Knopfdruck oder einmal beim Öffnen (Standard aus), Download mit Ladebalken und MB-Anzeige, SHA-256 gegen die von GitHub gemeldete Summe, „Installieren“ von Anfang an sichtbar aber gesperrt. Im Schonermodus nie Netzwerk.
  - Installer (Inno Setup) nach System32 mit gleicher AppId bei allen Versionen, optional „Als Bildschirmschoner einstellen“.
  - Schrift Inter (Regular, Bold) eingebettet; Housi-Logo oben links am Gerät (v0.3.1): aus der SVG-Datei Housi Logo Schwarz.svg als Resources\Logo.png in Silber #b8bdc6 gerendert (Schwarz wäre auf dem Gehäuse unsichtbar); build.bat bettet es automatisch ein.
- Abweichungen vom Regelwerk: kein Tipps-Fenster (Platz); Zoom per Mausrad nicht sinnvoll bei Vollbild, stattdessen Gerätegrösse; Rechtstexte ohne „Akzeptieren“ beim Anmelden (es gibt kein Profil); Inter nur in Regular und Bold (wie RoutedMidi); Changelog-Titel ohne Emoji.
- Noch nicht: Datum, weitere Modi, Installer-Test, Update-Test (es gibt noch kein GitHub-Release).
- Build: build.bat (Visual Studio mit C++-Werkzeugen, optional Inno Setup), Ergebnis in fertig\.

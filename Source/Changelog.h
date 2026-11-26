// ScreenSaver v0.3.2 · Build 7 · 2026-11-26 · Pre-Alpha (Phase 2)
// Changelog im Programm (Regelwerk: eine Liste im Programm, neueste Version zuerst; Typen neu, besser, fix, intern)
#pragma once
#include "Common.h"

struct ClItem { const wchar_t* type; const wchar_t* text; };
struct ClVersion { const wchar_t* version; const wchar_t* date; std::vector<ClItem> items; };

static const std::vector<ClVersion>& AppChangelog()
{
    static const std::vector<ClVersion> v =
    {
        { L"0.3.2", L"2026-11-26", {
            { L"neu",    L"Neuer Name: ScreenSaver (vorher Bildschirmschoner LED)" },
            { L"intern", L"Updates kommen von GitHub housi666/ScreenSaver; Einstellungen und Windows-Auswahl werden vom alten Namen übernommen" } } },
        { L"0.3.1", L"2026-11-26", {
            { L"neu",    L"Housi-Logo (Silber) oben links am Gerät, mit mehr Platz im Kopfbereich" } } },
        { L"0.3.0", L"2026-11-26", {
            { L"neu",    L"Einstellungen im Carbon-Stil nach Regelwerk: glänzende Display-Knöpfe, Live-Vorschau, Support, Rechtstexte, Changelog, Phasen-Hinweis" },
            { L"neu",    L"Gerätegrösse einstellbar (30 bis 100 Prozent des Bildschirms)" },
            { L"neu",    L"Logo-Platz oben links am Gerät statt des Programmnamens" },
            { L"neu",    L"Installer (Inno Setup) installiert den Bildschirmschoner nach Windows" },
            { L"neu",    L"Update-Funktion über GitHub mit Download, Prüfsumme und Installieren" },
            { L"neu",    L"Schrift Inter eingebettet" },
            { L"besser", L"Pixel-Shift bleibt immer im freien Rand, unabhängig von der Gerätegrösse" } } },
        { L"0.2.1", L"2026-11-26", {
            { L"fix",    L"Build-Fehler behoben (min/max- und DrawText-Makros von windows.h)" },
            { L"intern", L"Start-Dateien Test starten.bat und Einstellungen.bat" } } },
        { L"0.2.0", L"2026-11-26", {
            { L"neu",    L"Bildschirmschoner als .scr mit Vollbild, Einstellungen und Vorschau" },
            { L"neu",    L"Carbon-Gerät mit Glasdisplay und LED-Matrix 16 × 64" },
            { L"neu",    L"Uhr mit wanderndem Farbverlauf, Sekundenleiste, fünf Farbverläufe, Tempo" },
            { L"neu",    L"Monitor-Auswahl und Pixel-Shift gegen Einbrennen" } } },
        { L"0.1.1", L"2026-11-26", {
            { L"intern", L"Konzept: keine feste Akzentfarbe, Monitore wählbar, kein Tipps-Fenster" } } },
        { L"0.1.0", L"2026-11-23", {
            { L"intern", L"Projektgerüst in Phase 0 (Konzept, RoadMap, Changelog)" } } }
    };
    return v;
}

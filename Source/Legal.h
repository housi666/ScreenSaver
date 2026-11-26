// ScreenSaver v0.3.2 · Build 7 · 2026-11-26 · Pre-Alpha (Phase 2)
// Rechtstexte (Vorlagen, keine Rechtsberatung). Fassung siehe BSL_LEGAL_DATE in Version.h
#pragma once
#include "Common.h"

static const wchar_t* LegalTerms()
{
    return
        L"Nutzungsbedingungen\n\n"
        L"1. Worum es geht\n"
        L"ScreenSaver ist ein kostenloses Programm von Housi Guntern. Es wurde mit Claude (KI) erstellt und ist ein privates Projekt.\n\n"
        L"2. Entwicklungsstand\n"
        L"Das Programm befindet sich in einer frühen Phase (Pre-Alpha). Es kann Fehler enthalten und sich jederzeit ändern.\n\n"
        L"3. Nutzung auf eigene Verantwortung\n"
        L"Das Programm wird so zur Verfügung gestellt, wie es ist, ohne Zusicherung bestimmter Eigenschaften. Soweit gesetzlich zulässig, wird keine Haftung für Schäden übernommen, die durch die Nutzung entstehen.\n\n"
        L"4. Installation\n"
        L"Der Installer legt die Datei ScreenSaver.scr im Windows-Ordner ab. Die Deinstallation entfernt sie wieder.\n\n"
        L"5. Support\n"
        L"Fragen und Probleme über Instagram oder TikTok (housi_666). Ein Anspruch auf Support besteht nicht.\n\n"
        L"Hinweis: Dieser Text ist eine Vorlage und keine Rechtsberatung.";
}

static const wchar_t* LegalPrivacy()
{
    return
        L"Datenschutzerklärung\n\n"
        L"1. Was gespeichert wird\n"
        L"Das Programm speichert nur deine Einstellungen (Farbverlauf, Tempo, Grösse, Monitore und Schalter) lokal auf deinem Rechner in der Datei %APPDATA%\\ScreenSaver\\Einstellungen.ini. Es gibt kein Konto, keine Anmeldung, keine Statistik und keine Werbung.\n\n"
        L"2. Update-Prüfung\n"
        L"Nur wenn du \"Update suchen\" drückst oder \"Automatische Updates\" einschaltest (Standard: aus), fragt das Programm bei GitHub (api.github.com) nach der neuesten Version. Dabei sieht GitHub deine IP-Adresse. Es werden keine weiteren Daten von dir gesendet. Heruntergeladen oder installiert wird nie automatisch.\n\n"
        L"3. Support über Social Media\n"
        L"Wenn du mich über Instagram oder TikTok kontaktierst, gelten die Datenschutzbestimmungen dieser Anbieter.\n\n"
        L"4. Deine Rechte\n"
        L"Du kannst die Einstellungsdatei jederzeit löschen. Mit der Deinstallation verschwindet das Programm.\n\n"
        L"Hinweis: Dieser Text ist eine Vorlage und keine Rechtsberatung.";
}

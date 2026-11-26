// ScreenSaver v0.3.2 · Build 7 · 2026-11-26 · Pre-Alpha (Phase 2)
// Gemeinsame Teile: Einbindungen, Einstellungen, Monitore, Farbverläufe, LED-Matrix
#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX       // sonst kollidieren min/max-Makros mit std::min/std::max
#define NODRAWTEXT     // sonst wird DrawText zu DrawTextW und passt nicht zu Direct2D
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wincodec.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <d2d1.h>
#include <dwrite_3.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <cstdint>
#include <cstdlib>
#include <cwctype>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "Version.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

template <class T> static void SafeRelease (T*& p) { if (p) { p->Release(); p = nullptr; } }

// Ressourcen-Nummern (siehe app.rc)
enum { ResFontRegular = 101, ResFontBold = 102, ResLogo = 103 };

// ---------------------------------------------------------------------------------------------
// Einstellungen (Datei %APPDATA%\ScreenSaver\Einstellungen.ini)
// ---------------------------------------------------------------------------------------------
struct Settings
{
    int  palette    = 0;      // 0 Regenbogen, 1 Feuer, 2 Eis, 3 Metal, 4 Grün
    int  speed      = 5;      // 1..10
    int  size       = 80;     // Gerätegrösse in Prozent des Bildschirms, 30..100
    bool secondsBar = true;   // Sekundenleiste in der untersten LED-Zeile
    bool pixelShift = true;   // Gerät wandert langsam (gegen Einbrennen)
    bool autoUpdate = false;  // beim Öffnen der Einstellungen einmal nach Updates suchen
    std::wstring monitors;    // ausgewählte Monitore (Gerätenamen, getrennt durch ';'), leer = nur Hauptmonitor
};

static const wchar_t* kPaletteNames[] = { L"Regenbogen", L"Feuer", L"Eis", L"Metal", L"Grün und Petrol" };
static const int kPaletteCount = 5;

static std::wstring SettingsPath()
{
    wchar_t dir[MAX_PATH] = {};
    SHGetFolderPathW (nullptr, CSIDL_APPDATA, nullptr, 0, dir);
    std::wstring d = std::wstring (dir) + L"\\ScreenSaver";
    CreateDirectoryW (d.c_str(), nullptr);
    const std::wstring ini = d + L"\\Einstellungen.ini";
    // Übernahme der Einstellungen vom alten Namen "BildschirmschonerLED" (bis v0.3.1)
    if (GetFileAttributesW (ini.c_str()) == INVALID_FILE_ATTRIBUTES)
        CopyFileW ((std::wstring (dir) + L"\\BildschirmschonerLED\\Einstellungen.ini").c_str(), ini.c_str(), TRUE);
    return ini;
}

static Settings LoadSettings()
{
    Settings s;
    const std::wstring p = SettingsPath();
    s.palette    = std::clamp ((int) GetPrivateProfileIntW (L"Anzeige", L"Palette", 0, p.c_str()), 0, kPaletteCount - 1);
    s.speed      = std::clamp ((int) GetPrivateProfileIntW (L"Anzeige", L"Tempo", 5, p.c_str()), 1, 10);
    s.size       = std::clamp ((int) GetPrivateProfileIntW (L"Anzeige", L"Groesse", 80, p.c_str()), 30, 100);
    s.secondsBar = GetPrivateProfileIntW (L"Anzeige", L"Sekundenleiste", 1, p.c_str()) != 0;
    s.pixelShift = GetPrivateProfileIntW (L"Anzeige", L"Verschieben", 1, p.c_str()) != 0;
    s.autoUpdate = GetPrivateProfileIntW (L"Update", L"Automatisch", 0, p.c_str()) != 0;
    wchar_t buf[1024] = {};
    GetPrivateProfileStringW (L"Anzeige", L"Monitore", L"", buf, 1024, p.c_str());
    s.monitors = buf;
    return s;
}

static void SaveSettings (const Settings& s)
{
    const std::wstring p = SettingsPath();
    WritePrivateProfileStringW (L"Anzeige", L"Palette", std::to_wstring (s.palette).c_str(), p.c_str());
    WritePrivateProfileStringW (L"Anzeige", L"Tempo", std::to_wstring (s.speed).c_str(), p.c_str());
    WritePrivateProfileStringW (L"Anzeige", L"Groesse", std::to_wstring (s.size).c_str(), p.c_str());
    WritePrivateProfileStringW (L"Anzeige", L"Sekundenleiste", s.secondsBar ? L"1" : L"0", p.c_str());
    WritePrivateProfileStringW (L"Anzeige", L"Verschieben", s.pixelShift ? L"1" : L"0", p.c_str());
    WritePrivateProfileStringW (L"Anzeige", L"Monitore", s.monitors.c_str(), p.c_str());
    WritePrivateProfileStringW (L"Update", L"Automatisch", s.autoUpdate ? L"1" : L"0", p.c_str());
}

// ---------------------------------------------------------------------------------------------
// Monitore
// ---------------------------------------------------------------------------------------------
struct MonitorInfo
{
    RECT rect {};
    std::wstring device;
    bool primary = false;
};

static BOOL CALLBACK EnumMonProc (HMONITOR hm, HDC, LPRECT, LPARAM lp)
{
    MONITORINFOEXW mi {};
    mi.cbSize = sizeof (mi);
    if (GetMonitorInfoW (hm, &mi))
    {
        MonitorInfo m;
        m.rect = mi.rcMonitor;
        m.device = mi.szDevice;
        m.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
        ((std::vector<MonitorInfo>*) lp)->push_back (m);
    }
    return TRUE;
}

static std::vector<MonitorInfo> ListMonitors()
{
    std::vector<MonitorInfo> v;
    EnumDisplayMonitors (nullptr, nullptr, EnumMonProc, (LPARAM) &v);
    return v;
}

static bool IsMonitorSelected (const Settings& s, const MonitorInfo& m)
{
    if (s.monitors.empty())
        return m.primary;
    return (L";" + s.monitors + L";").find (L";" + m.device + L";") != std::wstring::npos;
}

// ---------------------------------------------------------------------------------------------
// Farben und Farbverläufe
// ---------------------------------------------------------------------------------------------
struct Col { float r, g, b; };

static Col FromHex (unsigned rgb) { return { ((rgb >> 16) & 255) / 255.f, ((rgb >> 8) & 255) / 255.f, (rgb & 255) / 255.f }; }
static Col Lerp (Col a, Col b, float k) { return { a.r + (b.r - a.r) * k, a.g + (b.g - a.g) * k, a.b + (b.b - a.b) * k }; }

static Col Hsv (float h, float s, float v)
{
    h = h - std::floor (h);
    const float x = h * 6.f;
    const int i = (int) x;
    const float f = x - i, p = v * (1.f - s), q = v * (1.f - s * f), t = v * (1.f - s * (1.f - f));
    switch (i % 6)
    {
        case 0: return { v, t, p };
        case 1: return { q, v, p };
        case 2: return { p, v, t };
        case 3: return { p, q, v };
        case 4: return { t, p, v };
        default: return { v, p, q };
    }
}

// t beliebig, wird zyklisch behandelt
static Col PaletteColour (int pal, float t)
{
    t -= std::floor (t);
    if (pal == 0)
        return Hsv (t, 0.85f, 1.f);

    static const std::vector<std::vector<unsigned>> stops =
    {
        { 0xff2a00, 0xff8a00, 0xffd23a, 0xff8a00 },             // Feuer
        { 0x4dd4ff, 0x2a6bff, 0x9040f0, 0x2a6bff },             // Eis
        { 0xff2d78, 0x8a1538, 0x9040f0, 0xd01fd0 },             // Metal
        { 0x3dff7a, 0x1ba3a8, 0x22b04a, 0x1ba3a8 }              // Grün und Petrol
    };
    const auto& st = stops[(size_t) std::clamp (pal - 1, 0, (int) stops.size() - 1)];
    const float x = t * (float) st.size();
    const int i = (int) x;
    const float k = x - (float) i;
    return Lerp (FromHex (st[(size_t) i % st.size()]), FromHex (st[(size_t) (i + 1) % st.size()]), k);
}

// ---------------------------------------------------------------------------------------------
// LED-Schrift 5 x 7 (Ziffern und Doppelpunkt) und LED-Matrix
// ---------------------------------------------------------------------------------------------
static const uint8_t kDigits[10][7] =
{
    { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E },   // 0
    { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E },   // 1
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F },   // 2
    { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E },   // 3
    { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 },   // 4
    { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E },   // 5
    { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E },   // 6
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 },   // 7
    { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E },   // 8
    { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C }    // 9
};

static constexpr int kRows = 16, kCols = 64;

struct LedMatrix
{
    bool on[kRows][kCols] = {};

    void set (int r, int c) { if (r >= 0 && r < kRows && c >= 0 && c < kCols) on[r][c] = true; }

    void digit (int d, int col0, int row0, int scale)
    {
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (kDigits[d][y] & (0x10 >> x))
                    for (int sy = 0; sy < scale; ++sy)
                        for (int sx = 0; sx < scale; ++sx)
                            set (row0 + y * scale + sy, col0 + x * scale + sx);
    }

    void colon (int col0, int row0, int scale)
    {
        for (int y : { 1, 5 })
            for (int sy = 0; sy < scale; ++sy)
                for (int sx = 0; sx < scale; ++sx)
                    set (row0 + y * scale + sy, col0 + sx);
    }
};

// Uhr "HH:MM" in doppelter Grösse (Zeilen 1..14), Doppelpunkt blinkt, Sekundenleiste in Zeile 15
static void BuildClock (LedMatrix& m, const SYSTEMTIME& st, bool secondsBar)
{
    const int h = st.wHour, mi = st.wMinute;
    const int x = 7;
    m.digit (h / 10, x,      1, 2);
    m.digit (h % 10, x + 12, 1, 2);
    if (st.wMilliseconds < 600)
        m.colon (x + 24, 1, 2);
    m.digit (mi / 10, x + 28, 1, 2);
    m.digit (mi % 10, x + 40, 1, 2);

    if (secondsBar)
    {
        const float secf = (float) st.wSecond + (float) st.wMilliseconds / 1000.f;
        const int n = (int) std::floor (secf / 60.f * (float) kCols);
        for (int c = 0; c < n; ++c)
            m.set (15, c);
    }
}

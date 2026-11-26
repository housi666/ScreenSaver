// ScreenSaver v0.3.2 · Build 7 · 2026-11-26 · Pre-Alpha (Phase 2)
// Einstellungsfenster im Carbon-Stil nach Regelwerk (Direct2D, eigene Bedienelemente):
// glänzende Display-Knöpfe in eigenen Farben mit Klick-Animation, Support, Rechtstexte, Changelog,
// Phasen-Abzeichen, Update. Kein Tipps-Fenster (Abweichung, siehe Konzept.md).
#pragma once
#include "Gfx.h"
#include "Changelog.h"
#include "Legal.h"
#include "Update.h"

static constexpr float kUiW = 940.f, kUiH = 660.f;

enum BtnId
{
    B_None = 0, B_Close = 1, B_Support, B_Update, B_Phase, B_ModalClose,
    B_Pal0 = 10,                       // 10..14
    B_TempoMinus = 20, B_TempoPlus, B_SizeMinus, B_SizePlus, B_Seconds, B_Shift,
    B_Mon0 = 40,                       // 40..47
    B_Insta = 60, B_TikTok, B_Terms, B_Privacy, B_Changelog, B_TabTerms, B_TabPriv, B_ExpandAll, B_CollapseAll,
    B_UpdAuto, B_UpdCheck, B_UpdDownload, B_UpdCancel, B_UpdInstall,
    B_Node0 = 100                      // Changelog-Zeilen
};

enum ModalKind { M_None = 0, M_Support, M_Legal, M_Changelog, M_Update, M_Phase };

struct Btn
{
    D2D1_RECT_F r {};
    std::wstring text;
    unsigned col = 0x2a6bff;
    int  id = B_None;
    bool lit = false, enabled = true, flat = false, left = false;
    int  pal = -1;                                  // >= 0: Farbverlauf-Streifen zeichnen
    D2D1_RECT_F clip { -1e9f, -1e9f, 1e9f, 1e9f };
};

struct UiApp
{
    HWND hwnd = nullptr;
    Renderer R;
    Settings S;
    std::vector<MonitorInfo> mons;
    float dpi = 1.f;
    bool anim = true;

    int   modal = M_None, legalTab = 0;
    float scroll = 0.f, contentH = 0.f;
    D2D1_RECT_F viewport {}, panel {};
    std::set<std::wstring> open;
    std::vector<std::wstring> nodeKeys;

    std::vector<Btn> curBg, lastBg, curMd, lastMd;
    int   hover = -1, press = -1;
    float mx = -1.f, my = -1.f;
    float shineX = -1000.f;
    bool  tracking = false;
};

static unsigned ToHex (Col c)
{
    auto u = [] (float v) { return (unsigned) std::clamp (v * 255.f + 0.5f, 0.f, 255.f); };
    return (u (c.r) << 16) | (u (c.g) << 8) | u (c.b);
}

static ID2D1LinearGradientBrush* GradVC (ID2D1RenderTarget* rt, float y0, float y1, Col a, float aa, Col b, float ba)
{
    D2D1_GRADIENT_STOP gs[2] = { { 0.f, D2D1::ColorF (a.r, a.g, a.b, aa) }, { 1.f, D2D1::ColorF (b.r, b.g, b.b, ba) } };
    ID2D1GradientStopCollection* sc = nullptr;
    ID2D1LinearGradientBrush* br = nullptr;
    if (SUCCEEDED (rt->CreateGradientStopCollection (gs, 2, &sc)))
    {
        rt->CreateLinearGradientBrush (D2D1::LinearGradientBrushProperties (D2D1::Point2F (0.f, y0), D2D1::Point2F (0.f, y1)), sc, &br);
        sc->Release();
    }
    return br;
}

static bool InRect (const D2D1_RECT_F& r, float x, float y) { return x >= r.left && x <= r.right && y >= r.top && y <= r.bottom; }

static float MeasureH (const std::wstring& s, IDWriteTextFormat* f, float w)
{
    IDWriteTextLayout* l = nullptr;
    float h = 20.f;
    if (SUCCEEDED (gDW->CreateTextLayout (s.c_str(), (UINT32) s.size(), f, w, 100000.f, &l)) && l)
    {
        DWRITE_TEXT_METRICS m {};
        l->GetMetrics (&m);
        h = m.height;
        l->Release();
    }
    return h;
}

// ---------------------------------------------------------------------------------------------
// Display-Knopf: dunkle Glasfläche, Glanzschicht, gemeinsamer Lichtstreifen, Klick-Rückmeldung
// ---------------------------------------------------------------------------------------------
static void DrawShine (UiApp& A, D2D1_RECT_F r)
{
    if (! A.anim)
        return;
    const float h = r.bottom - r.top;
    const float skew = h * 0.577f;            // 30 Grad Neigung
    const float wide = 46.f, narrow = 14.f;
    if (A.shineX + wide + skew < r.left || A.shineX - wide - skew > r.right)
        return;                                 // Streifen liegt nicht über diesem Knopf

    A.R.rt->PushAxisAlignedClip (D2D1::RectF (r.left + 1.f, r.top + 1.f, r.right - 1.f, r.bottom - 1.f), D2D1_ANTIALIAS_MODE_ALIASED);
    for (int pass = 0; pass < 2; ++pass)
    {
        const float w = pass == 0 ? wide : narrow;
        const float x0 = A.shineX - w * 0.5f;
        ID2D1PathGeometry* g = nullptr;
        if (SUCCEEDED (gD2D->CreatePathGeometry (&g)) && g)
        {
            ID2D1GeometrySink* sk = nullptr;
            if (SUCCEEDED (g->Open (&sk)) && sk)
            {
                sk->BeginFigure (D2D1::Point2F (x0 + skew, r.top), D2D1_FIGURE_BEGIN_FILLED);
                sk->AddLine (D2D1::Point2F (x0 + skew + w, r.top));
                sk->AddLine (D2D1::Point2F (x0 + w, r.bottom));
                sk->AddLine (D2D1::Point2F (x0, r.bottom));
                sk->EndFigure (D2D1_FIGURE_END_CLOSED);
                sk->Close();
                sk->Release();
                A.R.br->SetColor (D2D1::ColorF (1.f, 1.f, 1.f, pass == 0 ? 0.06f : 0.09f));
                A.R.rt->FillGeometry (g, A.R.br);
            }
            g->Release();
        }
    }
    A.R.rt->PopAxisAlignedClip();
}

static void DrawBtn (UiApp& A, const Btn& b, bool hover, bool pressed)
{
    ID2D1RenderTarget* rt = A.R.rt;
    ID2D1SolidColorBrush* br = A.R.br;
    const D2D1_RECT_F r = b.r;
    const Col c = FromHex (b.col);

    if (b.flat)
    {
        const unsigned tc = pressed ? b.col : (hover ? 0xffffff : 0xb8bdc6);
        Txt (A.R, b.text, Fmt (14.f, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_TEXT_ALIGNMENT_CENTER, true),
             D2D1::RectF (r.left, r.top + (pressed ? 1.f : 0.f), r.right, r.bottom + (pressed ? 1.f : 0.f)), tc);
        if (hover)
        {
            br->SetColor (D2D1::ColorF (tc, 0.8f));
            rt->DrawLine (D2D1::Point2F (r.left + 8, r.bottom - 5), D2D1::Point2F (r.right - 8, r.bottom - 5), br, 1.f);
        }
        return;
    }

    const D2D1_ROUNDED_RECT rr = D2D1::RoundedRect (r, 8.f, 8.f);
    float aTop = b.lit ? 0.50f : 0.16f, aBot = b.lit ? 0.32f : 0.07f;
    if (hover) { aTop += 0.08f; aBot += 0.05f; }
    if (! b.enabled) { aTop = 0.05f; aBot = 0.03f; }
    if (ID2D1LinearGradientBrush* g = GradVC (rt, r.top, r.bottom, Lerp (FromHex (0x0e1522), c, aTop), 1.f, Lerp (FromHex (0x070a10), c, aBot), 1.f))
    {
        rt->FillRoundedRectangle (rr, g);
        g->Release();
    }

    // Farbverlauf-Streifen für die Palettenknöpfe
    if (b.pal >= 0)
    {
        const float x0 = r.left + (r.right - r.left) * 0.42f, x1 = r.right - 8.f;
        const int steps = 28;
        const float sw = (x1 - x0) / (float) steps;
        for (int i = 0; i < steps; ++i)
        {
            const Col pc = PaletteColour (b.pal, (float) i / (float) steps * 0.8f);
            br->SetColor (D2D1::ColorF (pc.r, pc.g, pc.b, 1.f));
            rt->FillRectangle (D2D1::RectF (x0 + sw * (float) i, r.top + 7.f, x0 + sw * (float) (i + 1) + 0.6f, r.bottom - 7.f), br);
        }
    }

    // Glanzschicht (obere Hälfte)
    rt->PushAxisAlignedClip (D2D1::RectF (r.left + 1.f, r.top + 1.f, r.right - 1.f, r.top + (r.bottom - r.top) * 0.48f), D2D1_ANTIALIAS_MODE_ALIASED);
    if (ID2D1LinearGradientBrush* g = GradVC (rt, r.top, r.top + (r.bottom - r.top) * 0.48f, { 1, 1, 1 }, 0.12f, { 1, 1, 1 }, 0.0f))
    {
        rt->FillRoundedRectangle (rr, g);
        g->Release();
    }
    rt->PopAxisAlignedClip();

    if (b.enabled)
        DrawShine (A, r);

    // Rand: gedrückt = Farbe voll, sonst gedämpft
    if (! b.enabled)
        br->SetColor (D2D1::ColorF (0x3b3e45u));
    else if (pressed)
        br->SetColor (D2D1::ColorF (b.col, 1.f));
    else
        br->SetColor (D2D1::ColorF (b.col, b.lit ? 0.95f : 0.55f));
    rt->DrawRoundedRectangle (rr, br, pressed ? 2.f : 1.5f);

    // Text (gedrückt: 1 px nach unten, Farbe aufgehellt)
    unsigned tc = b.enabled ? (b.lit ? 0xffffff : 0xeef1f6) : 0x6b6e7c;
    if (pressed)
        tc = ToHex (Lerp (c, { 1, 1, 1 }, 0.35f));
    const float dy = pressed ? 1.f : 0.f;
    D2D1_RECT_F tr = D2D1::RectF (r.left + (b.left ? 12.f : 4.f), r.top + dy, r.right - 4.f, r.bottom + dy);
    if (b.pal >= 0)
        tr.right = r.left + (r.right - r.left) * 0.42f;
    const std::wstring label = b.text;
    if (b.pal >= 0)
        Txt (A.R, label, Fmt (14.f, DWRITE_FONT_WEIGHT_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING, true), D2D1::RectF (r.left + 14.f, r.top + dy, tr.right, r.bottom + dy), tc);
    else
        Txt (A.R, label, Fmt (14.f, DWRITE_FONT_WEIGHT_BOLD, b.left ? DWRITE_TEXT_ALIGNMENT_LEADING : DWRITE_TEXT_ALIGNMENT_CENTER, true), tr, tc);
}

static void AddBtn (UiApp& A, std::vector<Btn>& list, const Btn& b, bool live)
{
    const bool hover = live && b.id != B_None && b.enabled && A.hover == b.id;
    const bool pressed = hover && A.press == b.id;
    DrawBtn (A, b, hover, pressed);
    if (b.id != B_None && live)
        list.push_back (b);
}

static Btn MakeBtn (float x, float y, float w, float h, const std::wstring& text, unsigned col, int id, bool lit = false, bool enabled = true)
{
    Btn b;
    b.r = D2D1::RectF (x, y, x + w, y + h);
    b.text = text;
    b.col = col;
    b.id = id;
    b.lit = lit;
    b.enabled = enabled;
    return b;
}

static void Section (UiApp& A, float x, float y, const wchar_t* text, unsigned col)
{
    A.R.br->SetColor (D2D1::ColorF (col, 1.f));
    A.R.rt->FillRectangle (D2D1::RectF (x, y + 2.f, x + 4.f, y + 20.f), A.R.br);
    Txt (A.R, text, Fmt (13.f, DWRITE_FONT_WEIGHT_BOLD), D2D1::RectF (x + 12.f, y, x + 400.f, y + 22.f), 0x9aa3b2);
}

static void Card (UiApp& A, D2D1_RECT_F r, float radius = 12.f)
{
    const D2D1_ROUNDED_RECT rr = D2D1::RoundedRect (r, radius, radius);
    if (ID2D1LinearGradientBrush* g = GradV (A.R.rt, r.top, r.bottom, 0x162034, 1.f, 0x0c111b, 1.f))
    {
        A.R.rt->FillRoundedRectangle (rr, g);
        g->Release();
    }
    A.R.br->SetColor (D2D1::ColorF (0x3b3e45u));
    A.R.rt->DrawRoundedRectangle (rr, A.R.br, 1.f);
}

// ---------------------------------------------------------------------------------------------
// Hauptfenster (Hintergrundebene)
// ---------------------------------------------------------------------------------------------
static void DrawBackground (UiApp& A, double t, const D2D1::Matrix3x2F& base)
{
    ID2D1RenderTarget* rt = A.R.rt;
    const bool live = (A.modal == M_None);
    auto& LB = A.curBg;

    // Gehäuse-Hintergrund
    if (ID2D1LinearGradientBrush* g = GradV (rt, 0, kUiH, 0x26282d, 1.f, 0x1a1c1f, 1.f))
    {
        rt->FillRectangle (D2D1::RectF (0, 0, kUiW, kUiH), g);
        g->Release();
    }

    // Kopfzeile: Produktname, Version, Made by / Coded with, Phasen-Abzeichen, Update, Support
    Txt (A.R, L"SCREENSAVER", Fmt (20.f, DWRITE_FONT_WEIGHT_BOLD), D2D1::RectF (28, 12, 420, 40), 0xeef1f6);
    Txt (A.R, L"v" BSL_VERSION L" · Build " BSL_BUILD L" · Made by Housi · Coded with Claude", Fmt (12.f), D2D1::RectF (28, 42, 620, 62), 0x6b6e7c);
    AddBtn (A, LB, MakeBtn (344, 14, 96, 26, L"" BSL_PHASE, 0xffd23a, B_Phase), live);
    AddBtn (A, LB, MakeBtn (680, 14, 108, 34, L"Update", 0xff2d78, B_Update), live);
    AddBtn (A, LB, MakeBtn (796, 14, 116, 34, L"Support", 0x8a1538, B_Support), live);
    A.R.br->SetColor (D2D1::ColorF (0x3b3e45u));
    rt->DrawLine (D2D1::Point2F (28, 70), D2D1::Point2F (kUiW - 28, 70), A.R.br, 1.f);

    // Live-Vorschau: Bildschirm (Seitenverhältnis des Hauptmonitors) mit dem Gerät in der gewählten Grösse
    const D2D1_RECT_F card = D2D1::RectF (28, 84, 568, 316);
    Card (A, card);
    float aspect = 16.f / 9.f;
    for (const auto& m : A.mons)
        if (m.primary && m.rect.bottom > m.rect.top)
            aspect = (float) (m.rect.right - m.rect.left) / (float) (m.rect.bottom - m.rect.top);
    const float availW = (card.right - card.left) - 24.f, availH = (card.bottom - card.top) - 24.f;
    const float sw = std::min (availW, availH * aspect), sh = sw / aspect;
    const float sx = card.left + 12.f + (availW - sw) * 0.5f, sy = card.top + 12.f + (availH - sh) * 0.5f;
    A.R.br->SetColor (D2D1::ColorF (0x000000u));
    rt->FillRectangle (D2D1::RectF (sx, sy, sx + sw, sy + sh), A.R.br);
    const float f = (float) A.S.size / 100.f;
    const float dw = std::min (sw * f, sh * f * (kDevW / kDevH));
    const float dscale = dw / kDevW, dh = kDevH * dscale;
    DrawDevice (A.R, A.S, t, sx + (sw - dw) * 0.5f, sy + (sh - dh) * 0.5f, dscale, base);
    rt->SetTransform (base);
    A.R.br->SetColor (D2D1::ColorF (0x474b52u));
    rt->DrawRectangle (D2D1::RectF (sx, sy, sx + sw, sy + sh), A.R.br, 1.f);

    // Anzeige
    Section (A, 28, 336, L"ANZEIGE", 0xd01fd0);
    AddBtn (A, LB, MakeBtn (28, 360, 120, 38, L"Uhr", 0xd01fd0, B_None, true), live);
    Txt (A.R, L"Weitere Modi (Laufschrift, Plasma, Wellen) folgen.", Fmt (13.f), D2D1::RectF (164, 360, 568, 398), 0x6b6e7c);

    // Monitore
    Section (A, 28, 420, L"MONITORE", 0x22b04a);
    for (size_t i = 0; i < A.mons.size() && i < 8; ++i)
    {
        const auto& m = A.mons[i];
        std::wstring label = L"Monitor " + std::to_wstring (i + 1) + L"  ·  " + std::to_wstring (m.rect.right - m.rect.left)
                             + L" × " + std::to_wstring (m.rect.bottom - m.rect.top) + (m.primary ? L"  ·  Haupt" : L"");
        AddBtn (A, LB, MakeBtn (28 + (float) (i % 2) * 274, 444 + (float) (i / 2) * 44, 262, 36, label, 0x22b04a,
                               B_Mon0 + (int) i, IsMonitorSelected (A.S, m)), live);
    }

    // Rechte Spalte
    Section (A, 592, 84, L"FARBVERLAUF", 0xd01fd0);
    for (int i = 0; i < kPaletteCount; ++i)
    {
        Btn b = MakeBtn (592, 108 + (float) i * 42, 320, 34, kPaletteNames[i], ToHex (PaletteColour (i, 0.15f)), B_Pal0 + i, A.S.palette == i);
        b.pal = i;
        AddBtn (A, LB, b, live);
    }

    Section (A, 592, 326, L"TEMPO", 0x2a6bff);
    AddBtn (A, LB, MakeBtn (592, 350, 60, 38, L"−", 0x2a6bff, B_TempoMinus, false, A.S.speed > 1), live);
    AddBtn (A, LB, MakeBtn (660, 350, 184, 38, std::to_wstring (A.S.speed) + L" / 10", 0x2a6bff, B_None, true), live);
    AddBtn (A, LB, MakeBtn (852, 350, 60, 38, L"+", 0x2a6bff, B_TempoPlus, false, A.S.speed < 10), live);

    Section (A, 592, 402, L"GRÖSSE", 0x4b3fd0);
    AddBtn (A, LB, MakeBtn (592, 426, 60, 38, L"−", 0x4b3fd0, B_SizeMinus, false, A.S.size > 30), live);
    AddBtn (A, LB, MakeBtn (660, 426, 184, 38, std::to_wstring (A.S.size) + L" %", 0x4b3fd0, B_None, true), live);
    AddBtn (A, LB, MakeBtn (852, 426, 60, 38, L"+", 0x4b3fd0, B_SizePlus, false, A.S.size < 100), live);

    Section (A, 592, 478, L"OPTIONEN", 0x1ba3a8);
    AddBtn (A, LB, MakeBtn (592, 502, 154, 38, L"Sekundenleiste", 0x1ba3a8, B_Seconds, A.S.secondsBar), live);
    AddBtn (A, LB, MakeBtn (758, 502, 154, 38, L"Verschieben", 0x9040f0, B_Shift, A.S.pixelShift), live);

    Txt (A.R, L"Änderungen werden sofort gespeichert.", Fmt (13.f), D2D1::RectF (592, 552, 912, 572), 0x6b6e7c);
    AddBtn (A, LB, MakeBtn (782, 606, 130, 38, L"Schliessen", 0x6b6e7c, B_Close), live);
}

// ---------------------------------------------------------------------------------------------
// Fenster (Modal)
// ---------------------------------------------------------------------------------------------
static void BeginModal (UiApp& A, const wchar_t* title, float w, float h)
{
    ID2D1RenderTarget* rt = A.R.rt;
    A.R.br->SetColor (D2D1::ColorF (0x000000u, 0.62f));
    rt->FillRectangle (D2D1::RectF (0, 0, kUiW, kUiH), A.R.br);
    A.panel = D2D1::RectF ((kUiW - w) * 0.5f, (kUiH - h) * 0.5f, (kUiW + w) * 0.5f, (kUiH + h) * 0.5f);
    Card (A, A.panel, 14.f);
    Txt (A.R, title, Fmt (20.f, DWRITE_FONT_WEIGHT_BOLD), D2D1::RectF (A.panel.left + 24, A.panel.top + 16, A.panel.right - 80, A.panel.top + 48), 0xeef1f6);
    AddBtn (A, A.curMd, MakeBtn (A.panel.right - 24 - 36, A.panel.top + 14, 36, 32, L"×", 0xff2d78, B_ModalClose), true);
}

static void DrawScrollbar (UiApp& A)
{
    const float viewH = A.viewport.bottom - A.viewport.top;
    if (A.contentH <= viewH + 1.f)
        return;
    const float thumbH = std::max (24.f, viewH * viewH / A.contentH);
    const float range = A.contentH - viewH;
    const float ty = A.viewport.top + (viewH - thumbH) * (A.scroll / range);
    A.R.br->SetColor (D2D1::ColorF (0x6b6e7cu, 0.55f));
    A.R.rt->FillRoundedRectangle (D2D1::RoundedRect (D2D1::RectF (A.viewport.right + 4, ty, A.viewport.right + 8, ty + thumbH), 2.f, 2.f), A.R.br);
}

static void DrawSupport (UiApp& A)
{
    BeginModal (A, L"Support & Social Media", 560, 360);
    const float px = A.panel.left, py = A.panel.top;
    Txt (A.R, L"Fragen, Probleme oder Ideen zu ScreenSaver? Schreib mir über einen dieser Kanäle – das ist gleichzeitig der Support-Kontakt.",
         Fmt (15.f), D2D1::RectF (px + 24, py + 64, px + 536, py + 130), 0xb8bdc6);

    const float by = py + 150;
    AddBtn (A, A.curMd, MakeBtn (px + 24, by, 244, 60, L"", 0xd01fd0, B_Insta), true);
    AddBtn (A, A.curMd, MakeBtn (px + 292, by, 244, 60, L"", 0x1ba3a8, B_TikTok), true);

    // Symbole (Vektor): Instagram = Rahmen, Kreis, Punkt; TikTok = Note
    auto* rt = A.R.rt;
    auto* br = A.R.br;
    br->SetColor (D2D1::ColorF (0xffffffu));
    const float ix = px + 44, iy = by + 16;
    rt->DrawRoundedRectangle (D2D1::RoundedRect (D2D1::RectF (ix, iy, ix + 28, iy + 28), 8.f, 8.f), br, 2.f);
    rt->DrawEllipse (D2D1::Ellipse (D2D1::Point2F (ix + 14, iy + 14), 6.5f, 6.5f), br, 2.f);
    rt->FillEllipse (D2D1::Ellipse (D2D1::Point2F (ix + 22, iy + 6), 1.8f, 1.8f), br);
    const float tx = px + 312, ty = by + 16;
    rt->FillEllipse (D2D1::Ellipse (D2D1::Point2F (tx + 9, ty + 22), 6.f, 4.6f), br);
    rt->DrawLine (D2D1::Point2F (tx + 14, ty + 22), D2D1::Point2F (tx + 14, ty + 3), br, 3.f);
    rt->DrawLine (D2D1::Point2F (tx + 14, ty + 3), D2D1::Point2F (tx + 24, ty + 9), br, 3.f);
    Txt (A.R, L"Instagram", Fmt (15.f, DWRITE_FONT_WEIGHT_BOLD), D2D1::RectF (px + 84, by + 8, px + 264, by + 32), 0xeef1f6);
    Txt (A.R, L"@housi_666", Fmt (13.f), D2D1::RectF (px + 84, by + 31, px + 264, by + 52), 0xb8bdc6);
    Txt (A.R, L"TikTok", Fmt (15.f, DWRITE_FONT_WEIGHT_BOLD), D2D1::RectF (px + 352, by + 8, px + 532, by + 32), 0xeef1f6);
    Txt (A.R, L"@housi_666", Fmt (13.f), D2D1::RectF (px + 352, by + 31, px + 532, by + 52), 0xb8bdc6);

    // Links: Nutzungsbedingungen · Datenschutzerklärung · Changelog
    const float ly = py + 360 - 64;
    Btn terms = MakeBtn (px + 24, ly, 170, 34, L"Nutzungsbedingungen", 0xb8bdc6, B_Terms);
    terms.flat = true;
    Btn priv = MakeBtn (px + 214, ly, 170, 34, L"Datenschutzerklärung", 0xb8bdc6, B_Privacy);
    priv.flat = true;
    Btn chg = MakeBtn (px + 404, ly, 100, 34, L"Changelog", 0xb8bdc6, B_Changelog);
    chg.flat = true;
    AddBtn (A, A.curMd, terms, true);
    AddBtn (A, A.curMd, priv, true);
    AddBtn (A, A.curMd, chg, true);
    Txt (A.R, L"·", Fmt (14.f, DWRITE_FONT_WEIGHT_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER, true), D2D1::RectF (px + 194, ly, px + 214, ly + 34), 0x6b6e7c);
    Txt (A.R, L"·", Fmt (14.f, DWRITE_FONT_WEIGHT_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER, true), D2D1::RectF (px + 384, ly, px + 404, ly + 34), 0x6b6e7c);
}

static void DrawLegal (UiApp& A)
{
    BeginModal (A, L"Nutzungsbedingungen · Datenschutz", 760, 560);
    const float px = A.panel.left, py = A.panel.top, pw = 760.f, ph = 560.f;
    AddBtn (A, A.curMd, MakeBtn (px + 24, py + 60, 200, 34, L"Nutzungsbedingungen", 0x4b3fd0, B_TabTerms, A.legalTab == 0), true);
    AddBtn (A, A.curMd, MakeBtn (px + 232, py + 60, 150, 34, L"Datenschutz", 0x9040f0, B_TabPriv, A.legalTab == 1), true);
    Txt (A.R, L"Fassung vom " BSL_LEGAL_DATE, Fmt (13.f, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_TEXT_ALIGNMENT_TRAILING),
         D2D1::RectF (px + pw - 24 - 240, py + 68, px + pw - 24, py + 92), 0x6b6e7c);

    A.viewport = D2D1::RectF (px + 24, py + 108, px + pw - 36, py + ph - 24);
    const std::wstring text = A.legalTab == 0 ? LegalTerms() : LegalPrivacy();
    IDWriteTextFormat* fmt = Fmt (14.f);
    const float w = A.viewport.right - A.viewport.left;
    A.contentH = MeasureH (text, fmt, w) + 8.f;
    A.R.rt->PushAxisAlignedClip (A.viewport, D2D1_ANTIALIAS_MODE_ALIASED);
    Txt (A.R, text, fmt, D2D1::RectF (A.viewport.left, A.viewport.top - A.scroll, A.viewport.right, A.viewport.top - A.scroll + A.contentH + 10.f), 0xb8bdc6);
    A.R.rt->PopAxisAlignedClip();
    DrawScrollbar (A);
}

// Changelog: Hauptversion -> Nebenversion -> Version, aufklappbar
static void DrawChangelog (UiApp& A)
{
    BeginModal (A, L"Changelog", 760, 560);
    const float px = A.panel.left, py = A.panel.top, pw = 760.f, ph = 560.f;
    AddBtn (A, A.curMd, MakeBtn (px + 24, py + 60, 150, 34, L"Alle aufklappen", 0x22b04a, B_ExpandAll), true);
    AddBtn (A, A.curMd, MakeBtn (px + 182, py + 60, 150, 34, L"Alle zuklappen", 0x1ba3a8, B_CollapseAll), true);

    A.viewport = D2D1::RectF (px + 24, py + 108, px + pw - 36, py + ph - 24);
    const float vw = A.viewport.right - A.viewport.left;
    auto* rt = A.R.rt;
    A.nodeKeys.clear();

    // Gruppieren (neueste zuerst bleibt erhalten)
    struct Minor { std::wstring key; std::vector<const ClVersion*> vers; };
    struct Major { std::wstring key; std::vector<Minor> minors; int count = 0; };
    std::vector<Major> majors;
    for (const auto& v : AppChangelog())
    {
        const std::wstring ver = v.version;
        const size_t d1 = ver.find (L'.');
        const size_t d2 = d1 == std::wstring::npos ? d1 : ver.find (L'.', d1 + 1);
        const std::wstring mj = ver.substr (0, d1);
        const std::wstring mn = d2 == std::wstring::npos ? ver : ver.substr (0, d2);
        Major* M = nullptr;
        for (auto& m : majors) if (m.key == mj) M = &m;
        if (! M) { majors.push_back ({ mj, {}, 0 }); M = &majors.back(); }
        Minor* N = nullptr;
        for (auto& n : M->minors) if (n.key == mn) N = &n;
        if (! N) { M->minors.push_back ({ mn, {} }); N = &M->minors.back(); }
        N->vers.push_back (&v);
        M->count++;
    }

    rt->PushAxisAlignedClip (A.viewport, D2D1_ANTIALIAS_MODE_ALIASED);
    float y = A.viewport.top - A.scroll;
    const float rowH = 34.f;
    auto isOpen = [&] (const std::wstring& k) { return A.open.count (k) > 0; };
    auto addRow = [&] (float indent, const std::wstring& key, const std::wstring& text, unsigned col) -> Btn
    {
        const int id = B_Node0 + (int) A.nodeKeys.size();
        A.nodeKeys.push_back (key);
        Btn b = MakeBtn (A.viewport.left + indent, y, vw - indent, rowH - 4.f, std::wstring (isOpen (key) ? L"−  " : L"+  ") + text, col, id, isOpen (key));
        b.left = true;
        b.clip = A.viewport;
        return b;
    };

    for (const auto& M : majors)
    {
        Btn bm = addRow (0, M.key, L"Version " + M.key + L"   (" + std::to_wstring (M.count) + (M.count == 1 ? L" Version)" : L" Versionen)"), 0x4b3fd0);
        AddBtn (A, A.curMd, bm, true);
        y += rowH;
        if (! isOpen (M.key))
            continue;
        for (const auto& N : M.minors)
        {
            Btn bn = addRow (20, N.key, L"Version " + N.key, 0x2a6bff);
            AddBtn (A, A.curMd, bn, true);
            y += rowH;
            if (! isOpen (N.key))
                continue;
            for (const ClVersion* V : N.vers)
            {
                Btn bv = addRow (40, V->version, std::wstring (L"v") + V->version, 0x1ba3a8);
                AddBtn (A, A.curMd, bv, true);
                Txt (A.R, V->date, Fmt (12.f, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_TEXT_ALIGNMENT_TRAILING, true),
                     D2D1::RectF (bv.r.right - 160, bv.r.top, bv.r.right - 12, bv.r.bottom), 0x9aa3b2);
                y += rowH;
                if (! isOpen (V->version))
                    continue;
                for (const auto& it : V->items)
                {
                    const std::wstring type = it.type;
                    unsigned tcol = 0x6b6e7c;
                    if (type == L"neu") tcol = 0x22b04a;
                    else if (type == L"besser") tcol = 0x2a6bff;
                    else if (type == L"fix") tcol = 0xff4a4a;
                    const float ix = A.viewport.left + 60.f;
                    const float tw = vw - 60.f - 70.f - 8.f;
                    const float th = MeasureH (it.text, Fmt (13.f), tw);
                    const float ih = std::max (26.f, th + 8.f);
                    // Plakette
                    A.R.br->SetColor (D2D1::ColorF (tcol, 0.25f));
                    rt->FillRoundedRectangle (D2D1::RoundedRect (D2D1::RectF (ix, y + 3, ix + 62, y + 23), 5.f, 5.f), A.R.br);
                    A.R.br->SetColor (D2D1::ColorF (tcol, 0.9f));
                    rt->DrawRoundedRectangle (D2D1::RoundedRect (D2D1::RectF (ix, y + 3, ix + 62, y + 23), 5.f, 5.f), A.R.br, 1.f);
                    Txt (A.R, type, Fmt (11.f, DWRITE_FONT_WEIGHT_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER, true), D2D1::RectF (ix, y + 3, ix + 62, y + 23), 0xeef1f6);
                    Txt (A.R, it.text, Fmt (13.f), D2D1::RectF (ix + 72, y + 4, ix + 72 + tw, y + 4 + th + 4), 0xb8bdc6);
                    y += ih;
                }
                y += 4.f;
            }
        }
    }
    rt->PopAxisAlignedClip();
    A.contentH = (y + A.scroll) - A.viewport.top + 4.f;
    DrawScrollbar (A);

    if (AppChangelog().empty())
        Txt (A.R, L"Kein Changelog vorhanden.", Fmt (14.f), D2D1::RectF (A.viewport.left, A.viewport.top, A.viewport.right, A.viewport.top + 30), 0x9aa3b2);
}

static void DrawUpdate (UiApp& A)
{
    BeginModal (A, L"Update", 640, 400);
    const float px = A.panel.left, py = A.panel.top, pw = 640.f, ph = 400.f;
    const UpdPhase ph0 = (UpdPhase) gUpd.phase.load();
    const bool busy = gUpd.busy.load();

    AddBtn (A, A.curMd, MakeBtn (px + 24, py + 64, 300, 40, std::wstring (L"Automatische Updates: ") + (A.S.autoUpdate ? L"Ein" : L"Aus"), 0x22b04a, B_UpdAuto, A.S.autoUpdate), true);
    AddBtn (A, A.curMd, MakeBtn (px + 340, py + 64, 150, 40, L"Update suchen", 0x2a6bff, B_UpdCheck, false, ! busy), true);

    Txt (A.R, L"Installiert: v" BSL_VERSION L" · Build " BSL_BUILD, Fmt (14.f), D2D1::RectF (px + 24, py + 124, px + pw - 24, py + 148), 0x9aa3b2);
    std::wstring msg = gUpd.getMsg();
    if (msg.empty())
        msg = L"Noch nicht gesucht.";
    unsigned mc = 0xeef1f6;
    if (ph0 == UpdPhase::Error) mc = 0xff4a4a;
    else if (ph0 == UpdPhase::UpToDate || ph0 == UpdPhase::Ready) mc = 0x3dff7a;
    Txt (A.R, msg, Fmt (15.f, DWRITE_FONT_WEIGHT_BOLD), D2D1::RectF (px + 24, py + 152, px + pw - 24, py + 180), mc);

    // Download: Ladebalken, Prozent, MB, Abbrechen
    const float by = py + 196;
    if (ph0 == UpdPhase::Available)
    {
        AddBtn (A, A.curMd, MakeBtn (px + 24, by, 200, 44, L"Herunterladen", 0xff2d78, B_UpdDownload, false, ! busy), true);
    }
    else if (ph0 == UpdPhase::Downloading || ph0 == UpdPhase::Verifying)
    {
        const uint64_t got = gUpd.got.load(), total = gUpd.total.load();
        const float frac = total > 0 ? std::min (1.f, (float) ((double) got / (double) total)) : 0.f;
        A.R.br->SetColor (D2D1::ColorF (0x0a0f18u));
        A.R.rt->FillRoundedRectangle (D2D1::RoundedRect (D2D1::RectF (px + 24, by, px + pw - 24, by + 22), 6.f, 6.f), A.R.br);
        A.R.br->SetColor (D2D1::ColorF (0xff2d78u));
        A.R.rt->FillRoundedRectangle (D2D1::RoundedRect (D2D1::RectF (px + 24, by, px + 24 + (pw - 48) * frac, by + 22), 6.f, 6.f), A.R.br);
        wchar_t buf[96] = {};
        swprintf_s (buf, L"%d %%  ·  %.1f / %.1f MB", (int) (frac * 100.f + 0.5f), (double) got / 1048576.0, (double) total / 1048576.0);
        Txt (A.R, buf, Fmt (13.f), D2D1::RectF (px + 24, by + 28, px + pw - 24, by + 50), 0xb8bdc6);
        AddBtn (A, A.curMd, MakeBtn (px + 24, by + 60, 150, 38, L"Abbrechen", 0x8a1538, B_UpdCancel, false, ph0 == UpdPhase::Downloading), true);
    }

    // Installieren: von Anfang an sichtbar, aber gesperrt bis der Download fertig und die Summe geprüft ist
    AddBtn (A, A.curMd, MakeBtn (px + pw - 24 - 200, py + ph - 70, 200, 44, L"Installieren", 0x22b04a, B_UpdInstall, ph0 == UpdPhase::Ready, ph0 == UpdPhase::Ready), true);
    Txt (A.R, L"Es wird nie automatisch heruntergeladen oder installiert.", Fmt (12.f), D2D1::RectF (px + 24, py + ph - 56, px + pw - 240, py + ph - 24), 0x6b6e7c);
}

static void DrawPhase (UiApp& A)
{
    BeginModal (A, L"" BSL_PHASE, 520, 260);
    const float px = A.panel.left, py = A.panel.top;
    Txt (A.R, L"Dieses Tool wurde mit Claude (KI) erstellt.", Fmt (15.f, DWRITE_FONT_WEIGHT_BOLD), D2D1::RectF (px + 24, py + 68, px + 496, py + 96), 0xeef1f6);
    Txt (A.R, L"ScreenSaver ist in der Phase " BSL_PHASE L". Das Programm kann Fehler enthalten und sich noch stark ändern.",
         Fmt (14.f), D2D1::RectF (px + 24, py + 108, px + 496, py + 200), 0xb8bdc6);
}

// ---------------------------------------------------------------------------------------------
// Fenster-Logik
// ---------------------------------------------------------------------------------------------
static void ClampScroll (UiApp& A)
{
    const float viewH = A.viewport.bottom - A.viewport.top;
    A.scroll = std::clamp (A.scroll, 0.f, std::max (0.f, A.contentH - viewH));
}

static int HitTest (const UiApp& A, float x, float y)
{
    const auto& list = A.modal != M_None ? A.lastMd : A.lastBg;
    for (size_t i = list.size(); i-- > 0;)
    {
        const Btn& b = list[i];
        if (b.enabled && InRect (b.r, x, y) && InRect (b.clip, x, y))
            return b.id;
    }
    return -1;
}

static void UiFrame (UiApp& A)
{
    if (! A.R.ensure (A.hwnd))
        return;
    auto* rt = A.R.rt;
    rt->BeginDraw();
    const D2D1::Matrix3x2F base = D2D1::Matrix3x2F::Scale (A.dpi, A.dpi);
    rt->SetTransform (base);
    rt->Clear (D2D1::ColorF (0x1a1c1fu));

    const double t = (double) GetTickCount64() / 1000.0;
    A.shineX = A.anim ? (float) std::fmod (t * 160.0, 1240.0) - 150.f : -1000.f;
    A.hover = HitTest (A, A.mx, A.my);
    A.curBg.clear();
    A.curMd.clear();
    ClampScroll (A);

    DrawBackground (A, t, base);
    rt->SetTransform (base);
    switch (A.modal)
    {
        case M_Support:   DrawSupport (A);   break;
        case M_Legal:     DrawLegal (A);     break;
        case M_Changelog: DrawChangelog (A); break;
        case M_Update:    DrawUpdate (A);    break;
        case M_Phase:     DrawPhase (A);     break;
        default: break;
    }
    A.lastBg.swap (A.curBg);
    A.lastMd.swap (A.curMd);

    if (rt->EndDraw() == D2DERR_RECREATE_TARGET)
        A.R.release();
}

static void OpenUrl (const wchar_t* url) { ShellExecuteW (nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL); }

static void OpenModal (UiApp& A, int kind)
{
    A.modal = kind;
    A.scroll = 0.f;
    A.contentH = 0.f;
    A.press = -1;
    if (kind == M_Changelog)
    {
        A.open.clear();
        const auto& v = AppChangelog();
        if (! v.empty())
        {
            const std::wstring ver = v[0].version;
            const size_t d1 = ver.find (L'.');
            const size_t d2 = d1 == std::wstring::npos ? d1 : ver.find (L'.', d1 + 1);
            A.open.insert (ver.substr (0, d1));
            A.open.insert (d2 == std::wstring::npos ? ver : ver.substr (0, d2));
            A.open.insert (ver);
        }
    }
}

static void Activate (UiApp& A, int id)
{
    bool save = false;
    if (id >= B_Node0)
    {
        const size_t i = (size_t) (id - B_Node0);
        if (i < A.nodeKeys.size())
        {
            const std::wstring& k = A.nodeKeys[i];
            if (A.open.count (k)) A.open.erase (k); else A.open.insert (k);
        }
        return;
    }
    if (id >= B_Pal0 && id < B_Pal0 + kPaletteCount) { A.S.palette = id - B_Pal0; save = true; }
    else if (id >= B_Mon0 && id < B_Mon0 + 8)
    {
        const size_t i = (size_t) (id - B_Mon0);
        if (i < A.mons.size())
        {
            std::wstring list;
            bool any = false;
            for (size_t k = 0; k < A.mons.size(); ++k)
            {
                bool sel = IsMonitorSelected (A.S, A.mons[k]);
                if (k == i) sel = ! sel;
                if (sel)
                {
                    list += (any ? L";" : L"") + A.mons[k].device;
                    any = true;
                }
            }
            A.S.monitors = list;     // leer = nur Hauptmonitor
            save = true;
        }
    }
    else switch (id)
    {
        case B_Close:       DestroyWindow (A.hwnd); return;
        case B_Support:     OpenModal (A, M_Support); return;
        case B_Update:      OpenModal (A, M_Update); return;
        case B_Phase:       OpenModal (A, M_Phase); return;
        case B_ModalClose:  OpenModal (A, M_None); return;
        case B_TempoMinus:  A.S.speed = std::max (1, A.S.speed - 1); save = true; break;
        case B_TempoPlus:   A.S.speed = std::min (10, A.S.speed + 1); save = true; break;
        case B_SizeMinus:   A.S.size = std::max (30, A.S.size - 5); save = true; break;
        case B_SizePlus:    A.S.size = std::min (100, A.S.size + 5); save = true; break;
        case B_Seconds:     A.S.secondsBar = ! A.S.secondsBar; save = true; break;
        case B_Shift:       A.S.pixelShift = ! A.S.pixelShift; save = true; break;
        case B_Insta:       OpenUrl (L"https://www.instagram.com/housi_666/"); return;
        case B_TikTok:      OpenUrl (L"https://www.tiktok.com/@housi_666"); return;
        case B_Terms:       OpenModal (A, M_Legal); A.legalTab = 0; return;
        case B_Privacy:     OpenModal (A, M_Legal); A.legalTab = 1; return;
        case B_Changelog:   OpenModal (A, M_Changelog); return;
        case B_TabTerms:    A.legalTab = 0; A.scroll = 0.f; return;
        case B_TabPriv:     A.legalTab = 1; A.scroll = 0.f; return;
        case B_ExpandAll:
        {
            for (const auto& v : AppChangelog())
            {
                const std::wstring ver = v.version;
                const size_t d1 = ver.find (L'.');
                const size_t d2 = d1 == std::wstring::npos ? d1 : ver.find (L'.', d1 + 1);
                A.open.insert (ver.substr (0, d1));
                A.open.insert (d2 == std::wstring::npos ? ver : ver.substr (0, d2));
                A.open.insert (ver);
            }
            return;
        }
        case B_CollapseAll: A.open.clear(); A.scroll = 0.f; return;
        case B_UpdAuto:     A.S.autoUpdate = ! A.S.autoUpdate; save = true; break;
        case B_UpdCheck:    StartUpdateCheck(); return;
        case B_UpdDownload: StartUpdateDownload(); return;
        case B_UpdCancel:   gUpd.cancel = true; return;
        case B_UpdInstall:
            if (gUpd.phase.load() == (int) UpdPhase::Ready)
            {
                InstallUpdate();
                DestroyWindow (A.hwnd);
            }
            return;
        default: return;
    }
    if (save)
        SaveSettings (A.S);
}

static LRESULT CALLBACK UiProc (HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    UiApp* A = (UiApp*) GetWindowLongPtrW (h, GWLP_USERDATA);
    switch (msg)
    {
        case WM_NCCREATE:
            SetWindowLongPtrW (h, GWLP_USERDATA, (LONG_PTR) ((CREATESTRUCTW*) lp)->lpCreateParams);
            return DefWindowProcW (h, msg, wp, lp);

        case WM_CREATE:
            SetTimer (h, 1, 16, nullptr);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            if (A)
                UiFrame (*A);
            ValidateRect (h, nullptr);
            return 0;

        case WM_TIMER:
            InvalidateRect (h, nullptr, FALSE);
            return 0;

        case WM_SIZE:
            if (A && A->R.rt)
                A->R.rt->Resize (D2D1::SizeU (LOWORD (lp), HIWORD (lp)));
            return 0;

        case WM_MOUSEMOVE:
            if (A)
            {
                A->mx = (float) GET_X_LPARAM (lp) / A->dpi;
                A->my = (float) GET_Y_LPARAM (lp) / A->dpi;
                if (! A->tracking)
                {
                    TRACKMOUSEEVENT tme { sizeof (tme), TME_LEAVE, h, 0 };
                    TrackMouseEvent (&tme);
                    A->tracking = true;
                }
            }
            return 0;

        case WM_MOUSELEAVE:
            if (A)
            {
                A->mx = A->my = -1.f;
                A->tracking = false;
            }
            return 0;

        case WM_SETCURSOR:
            if (A && A->hover > 0 && LOWORD (lp) == HTCLIENT)
            {
                SetCursor (LoadCursorW (nullptr, IDC_HAND));
                return TRUE;
            }
            break;

        case WM_LBUTTONDOWN:
            if (A)
            {
                A->mx = (float) GET_X_LPARAM (lp) / A->dpi;
                A->my = (float) GET_Y_LPARAM (lp) / A->dpi;
                A->press = HitTest (*A, A->mx, A->my);
                SetCapture (h);
            }
            return 0;

        case WM_LBUTTONUP:
            if (A)
            {
                ReleaseCapture();
                A->mx = (float) GET_X_LPARAM (lp) / A->dpi;
                A->my = (float) GET_Y_LPARAM (lp) / A->dpi;
                const int hit = HitTest (*A, A->mx, A->my);
                const int pressed = A->press;
                A->press = -1;
                if (pressed != -1 && hit == pressed)
                    Activate (*A, pressed);                                   // ausgelöst beim Loslassen
                else if (pressed == -1 && A->modal != M_None && ! InRect (A->panel, A->mx, A->my))
                    OpenModal (*A, M_None);                                   // Klick neben das Fenster schliesst
            }
            return 0;

        case WM_MOUSEWHEEL:
            if (A && A->modal != M_None)
            {
                A->scroll -= (float) GET_WHEEL_DELTA_WPARAM (wp) / 120.f * 48.f;
                ClampScroll (*A);
            }
            return 0;

        case WM_KEYDOWN:
            if (A && wp == VK_ESCAPE && A->modal != M_None)
                OpenModal (*A, M_None);
            return 0;

        case WM_CLOSE:
            DestroyWindow (h);
            return 0;

        case WM_DESTROY:
            KillTimer (h, 1);
            if (A)
                A->R.release();
            PostQuitMessage (0);
            return 0;
    }
    return DefWindowProcW (h, msg, wp, lp);
}

static int RunUi (HINSTANCE hi)
{
    if (! InitGraphics())
        return 1;

    UiApp A;
    A.S = LoadSettings();
    A.mons = ListMonitors();
    HDC dc = GetDC (nullptr);
    A.dpi = (float) GetDeviceCaps (dc, LOGPIXELSX) / 96.f;
    ReleaseDC (nullptr, dc);
    if (A.dpi < 1.f) A.dpi = 1.f;
    BOOL anim = TRUE;
    SystemParametersInfoW (SPI_GETCLIENTAREAANIMATION, 0, &anim, 0);
    A.anim = anim != FALSE;     // bei "weniger Bewegung" kein Lichtstreifen

    WNDCLASSW wc {};
    wc.lpfnWndProc = UiProc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursorW (nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"HousiScreenSaverUi";
    RegisterClassW (&wc);

    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT r = { 0, 0, (LONG) (kUiW * A.dpi), (LONG) (kUiH * A.dpi) };
    AdjustWindowRect (&r, style, FALSE);
    const int w = r.right - r.left, hgt = r.bottom - r.top;
    A.hwnd = CreateWindowExW (0, wc.lpszClassName, L"ScreenSaver – Einstellungen", style,
                              (GetSystemMetrics (SM_CXSCREEN) - w) / 2, (GetSystemMetrics (SM_CYSCREEN) - hgt) / 2, w, hgt,
                              nullptr, nullptr, hi, &A);
    if (! A.hwnd)
        return 1;
    ShowWindow (A.hwnd, SW_SHOW);

    if (A.S.autoUpdate)
        StartUpdateCheck();      // nur suchen, nie automatisch herunterladen

    MSG msg;
    while (GetMessageW (&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage (&msg);
        DispatchMessageW (&msg);
    }
    return 0;
}

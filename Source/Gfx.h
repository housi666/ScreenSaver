// ScreenSaver v0.3.2 · Build 7 · 2026-11-26 · Pre-Alpha (Phase 2)
// Grafik: Direct2D/DirectWrite, Inter aus den Ressourcen, Logo, Gerät mit LED-Matrix
#pragma once
#include "Common.h"

static ID2D1Factory*        gD2D   = nullptr;
static IDWriteFactory*      gDW    = nullptr;
static IWICImagingFactory*  gWIC   = nullptr;
static IDWriteFontCollection* gFonts = nullptr;   // enthält Inter (Regular, Bold), sonst Systemschrift

static bool GetRes (int id, const void*& p, DWORD& n)
{
    HRSRC r = FindResourceW (nullptr, MAKEINTRESOURCEW (id), RT_RCDATA);
    if (! r)
        return false;
    HGLOBAL g = LoadResource (nullptr, r);
    if (! g)
        return false;
    n = SizeofResource (nullptr, r);
    p = LockResource (g);
    return p != nullptr && n > 0;
}

// Inter aus den eingebetteten Ressourcen laden (Windows 10 ab 1703); bei Fehler bleibt Segoe UI
static void LoadInter()
{
    IDWriteFactory5* f5 = nullptr;
    if (FAILED (gDW->QueryInterface (__uuidof (IDWriteFactory5), (void**) &f5)))
        return;
    IDWriteInMemoryFontFileLoader* loader = nullptr;
    if (SUCCEEDED (f5->CreateInMemoryFontFileLoader (&loader)))
    {
        f5->RegisterFontFileLoader (loader);   // Loader bleibt absichtlich bis zum Programmende bestehen
        IDWriteFontSetBuilder1* sb = nullptr;
        if (SUCCEEDED (f5->CreateFontSetBuilder (&sb)))
        {
            for (int id : { (int) ResFontRegular, (int) ResFontBold })
            {
                const void* p = nullptr;
                DWORD n = 0;
                IDWriteFontFile* ff = nullptr;
                if (GetRes (id, p, n) && SUCCEEDED (loader->CreateInMemoryFontFileReference (f5, p, n, nullptr, &ff)))
                {
                    sb->AddFontFile (ff);
                    ff->Release();
                }
            }
            IDWriteFontSet* fs = nullptr;
            if (SUCCEEDED (sb->CreateFontSet (&fs)))
            {
                IDWriteFontCollection1* c1 = nullptr;
                if (SUCCEEDED (f5->CreateFontCollectionFromFontSet (fs, &c1)))
                    gFonts = c1;
                fs->Release();
            }
            sb->Release();
        }
    }
    f5->Release();
}

static bool InitGraphics()
{
    if (gD2D)
        return true;
    if (FAILED (D2D1CreateFactory (D2D1_FACTORY_TYPE_SINGLE_THREADED, &gD2D)))
        return false;
    if (FAILED (DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory), (IUnknown**) &gDW)))
        return false;
    CoCreateInstance (CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS (&gWIC));
    LoadInter();
    return true;
}

// Textformate werden zwischengespeichert (Grösse, Gewicht, Ausrichtung, vertikal zentriert)
struct FmtEntry { float size; int weight; int align; bool vcenter; IDWriteTextFormat* f; };
static std::vector<FmtEntry> gFmts;

static IDWriteTextFormat* Fmt (float size, DWRITE_FONT_WEIGHT w = DWRITE_FONT_WEIGHT_REGULAR,
                               DWRITE_TEXT_ALIGNMENT al = DWRITE_TEXT_ALIGNMENT_LEADING, bool vcenter = false)
{
    for (auto& e : gFmts)
        if (e.size == size && e.weight == (int) w && e.align == (int) al && e.vcenter == vcenter)
            return e.f;

    IDWriteTextFormat* f = nullptr;
    HRESULT hr = gDW->CreateTextFormat (gFonts ? L"Inter" : L"Segoe UI", gFonts, w, DWRITE_FONT_STYLE_NORMAL,
                                        DWRITE_FONT_STRETCH_NORMAL, size, L"de-ch", &f);
    if (FAILED (hr) || ! f)
    {
        SafeRelease (f);
        hr = gDW->CreateTextFormat (L"Segoe UI", nullptr, w, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"de-ch", &f);
    }
    if (f)
    {
        f->SetTextAlignment (al);
        if (vcenter)
        {
            f->SetParagraphAlignment (DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            f->SetWordWrapping (DWRITE_WORD_WRAPPING_NO_WRAP);
        }
    }
    gFmts.push_back ({ size, (int) w, (int) al, vcenter, f });
    return f;
}

static void ReleaseGraphics()
{
    for (auto& e : gFmts)
        SafeRelease (e.f);
    gFmts.clear();
    SafeRelease (gFonts);
    SafeRelease (gWIC);
    SafeRelease (gDW);
    SafeRelease (gD2D);
}

struct Renderer
{
    ID2D1HwndRenderTarget* rt   = nullptr;
    ID2D1SolidColorBrush*  br   = nullptr;
    ID2D1Bitmap*           logo = nullptr;

    bool ensure (HWND h)
    {
        if (rt)
            return true;
        RECT rc {};
        GetClientRect (h, &rc);
        if (FAILED (gD2D->CreateHwndRenderTarget (D2D1::RenderTargetProperties(),
                                                  D2D1::HwndRenderTargetProperties (h, D2D1::SizeU ((UINT32) rc.right, (UINT32) rc.bottom)),
                                                  &rt)))
            return false;
        rt->SetDpi (96.f, 96.f);   // wir rechnen in Pixeln
        rt->CreateSolidColorBrush (D2D1::ColorF (0u), &br);
        createLogo();
        return br != nullptr;
    }

    // Logo (PNG) aus den Ressourcen; fehlt es, wird ein Textlogo gezeichnet
    void createLogo()
    {
        const void* p = nullptr;
        DWORD n = 0;
        if (! gWIC || ! GetRes (ResLogo, p, n))
            return;
        IWICStream* st = nullptr;
        IWICBitmapDecoder* dec = nullptr;
        IWICBitmapFrameDecode* fr = nullptr;
        IWICFormatConverter* cv = nullptr;
        if (SUCCEEDED (gWIC->CreateStream (&st)) && SUCCEEDED (st->InitializeFromMemory ((BYTE*) p, n))
            && SUCCEEDED (gWIC->CreateDecoderFromStream (st, nullptr, WICDecodeMetadataCacheOnLoad, &dec))
            && SUCCEEDED (dec->GetFrame (0, &fr))
            && SUCCEEDED (gWIC->CreateFormatConverter (&cv))
            && SUCCEEDED (cv->Initialize (fr, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut)))
            rt->CreateBitmapFromWicBitmap (cv, nullptr, &logo);
        SafeRelease (cv);
        SafeRelease (fr);
        SafeRelease (dec);
        SafeRelease (st);
    }

    void release() { SafeRelease (logo); SafeRelease (br); SafeRelease (rt); }
};

static ID2D1LinearGradientBrush* GradV (ID2D1RenderTarget* rt, float y0, float y1, unsigned c0, float a0, unsigned c1, float a1)
{
    D2D1_GRADIENT_STOP gs[2] = { { 0.f, D2D1::ColorF (c0, a0) }, { 1.f, D2D1::ColorF (c1, a1) } };
    ID2D1GradientStopCollection* sc = nullptr;
    ID2D1LinearGradientBrush* b = nullptr;
    if (SUCCEEDED (rt->CreateGradientStopCollection (gs, 2, &sc)))
    {
        rt->CreateLinearGradientBrush (D2D1::LinearGradientBrushProperties (D2D1::Point2F (0.f, y0), D2D1::Point2F (0.f, y1)), sc, &b);
        sc->Release();
    }
    return b;
}

static ID2D1LinearGradientBrush* GradH (ID2D1RenderTarget* rt, float x0, float x1, D2D1_COLOR_F c0, D2D1_COLOR_F c1)
{
    D2D1_GRADIENT_STOP gs[2] = { { 0.f, c0 }, { 1.f, c1 } };
    ID2D1GradientStopCollection* sc = nullptr;
    ID2D1LinearGradientBrush* b = nullptr;
    if (SUCCEEDED (rt->CreateGradientStopCollection (gs, 2, &sc)))
    {
        rt->CreateLinearGradientBrush (D2D1::LinearGradientBrushProperties (D2D1::Point2F (x0, 0.f), D2D1::Point2F (x1, 0.f)), sc, &b);
        sc->Release();
    }
    return b;
}

// Text in Rechteck (Farbe als RGB-Hex)
static void Txt (Renderer& R, const std::wstring& s, IDWriteTextFormat* f, D2D1_RECT_F r, unsigned rgb, float a = 1.f)
{
    if (! f || s.empty())
        return;
    R.br->SetColor (D2D1::ColorF (rgb, a));
    R.rt->DrawText (s.c_str(), (UINT32) s.size(), f, r, R.br);
}

// ---------------------------------------------------------------------------------------------
// Gerät: Carbon-Gehäuse, Logo, Glasdisplay mit LED-Matrix. Feste Einheiten 1300 x 500.
// ---------------------------------------------------------------------------------------------
static constexpr float kDevW = 1300.f, kDevH = 500.f;

static void DrawDevice (Renderer& R, const Settings& S, double t, float x0, float y0, float scale,
                        const D2D1::Matrix3x2F& base = D2D1::Matrix3x2F::Identity())
{
    ID2D1HwndRenderTarget* rt = R.rt;
    ID2D1SolidColorBrush* br = R.br;
    rt->SetTransform (D2D1::Matrix3x2F::Scale (scale, scale) * D2D1::Matrix3x2F::Translation (x0, y0) * base);

    // Gehäuse
    const D2D1_ROUNDED_RECT body = D2D1::RoundedRect (D2D1::RectF (0, 0, kDevW, kDevH), 26.f, 26.f);
    if (ID2D1LinearGradientBrush* g = GradV (rt, 0, kDevH, 0x35383e, 1.f, 0x1a1c1f, 1.f))
    {
        rt->FillRoundedRectangle (body, g);
        g->Release();
    }
    br->SetColor (D2D1::ColorF (0x474b52u));
    rt->DrawRoundedRectangle (body, br, 2.f);

    // Logo oben links (Housi, Silber wie der Gehäusedruck), ohne Bild als Text
    if (R.logo)
    {
        const D2D1_SIZE_F ls = R.logo->GetSize();
        const float h = 66.f;
        const float w = std::min (h * ls.width / std::max (1.f, ls.height), 300.f);
        rt->DrawBitmap (R.logo, D2D1::RectF (52, 10, 52 + w, 10 + h), 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }
    else
    {
        Txt (R, L"HOUSI", Fmt (28.f, DWRITE_FONT_WEIGHT_BOLD), D2D1::RectF (52, 28, 400, 68), 0xb8bdc6);
    }

    // Beschriftung unten (Gehäusedruck): links Made by / Coded with, rechts Version
    Txt (R, L"ScreenSaver · Made by Housi · Coded with Claude", Fmt (15.f), D2D1::RectF (52, 458, 800, 488), 0x6b6e7c);
    const std::wstring ver = L"v" + std::wstring (L"" BSL_VERSION) + L" · Build " + std::wstring (L"" BSL_BUILD);
    Txt (R, ver, Fmt (15.f, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_TEXT_ALIGNMENT_TRAILING), D2D1::RectF (800, 458, kDevW - 52, 488), 0x6b6e7c);

    // Blende
    const D2D1_ROUNDED_RECT bezel = D2D1::RoundedRect (D2D1::RectF (50, 90, 1250, 440), 14.f, 14.f);
    br->SetColor (D2D1::ColorF (0x050608u));
    rt->FillRoundedRectangle (bezel, br);
    br->SetColor (D2D1::ColorF (0x474b52u));
    rt->DrawRoundedRectangle (bezel, br, 1.5f);

    // Glas
    const D2D1_RECT_F glassR = D2D1::RectF (66, 106, 1234, 424);
    const D2D1_ROUNDED_RECT glass = D2D1::RoundedRect (glassR, 8.f, 8.f);
    if (ID2D1LinearGradientBrush* g = GradV (rt, glassR.top, glassR.bottom, 0x0e1522, 1.f, 0x070a10, 1.f))
    {
        rt->FillRoundedRectangle (glass, g);
        g->Release();
    }

    // LED-Matrix
    LedMatrix m;
    SYSTEMTIME st {};
    GetLocalTime (&st);
    BuildClock (m, st, S.secondsBar);

    const float padX = 28.f, padY = 28.f;
    const float gx = glassR.left + padX, gy = glassR.top + padY;
    const float pitchX = (glassR.right - glassR.left - 2 * padX) / (float) kCols;
    const float pitchY = (glassR.bottom - glassR.top - 2 * padY) / (float) kRows;
    const float rad = std::min (pitchX, pitchY) * 0.36f;
    const float tf = (float) t * (float) S.speed * 0.01f;      // Zyklen pro Sekunde

    for (int r = 0; r < kRows; ++r)
    {
        for (int c = 0; c < kCols; ++c)
        {
            const float cx = gx + ((float) c + 0.5f) * pitchX;
            const float cy = gy + ((float) r + 0.5f) * pitchY;
            const Col col = PaletteColour (S.palette, (float) c / (float) kCols * 0.8f + (float) r / (float) kRows * 0.1f - tf);

            if (m.on[r][c])
            {
                // Schein, Kern, Glanzpunkt
                br->SetColor (D2D1::ColorF (col.r, col.g, col.b, 0.10f));
                rt->FillEllipse (D2D1::Ellipse (D2D1::Point2F (cx, cy), rad * 2.1f, rad * 2.1f), br);
                br->SetColor (D2D1::ColorF (col.r, col.g, col.b, 0.22f));
                rt->FillEllipse (D2D1::Ellipse (D2D1::Point2F (cx, cy), rad * 1.45f, rad * 1.45f), br);
                br->SetColor (D2D1::ColorF (col.r, col.g, col.b, 1.f));
                rt->FillEllipse (D2D1::Ellipse (D2D1::Point2F (cx, cy), rad, rad), br);
                br->SetColor (D2D1::ColorF (1.f, 1.f, 1.f, 0.30f));
                rt->FillEllipse (D2D1::Ellipse (D2D1::Point2F (cx - rad * 0.30f, cy - rad * 0.32f), rad * 0.38f, rad * 0.30f), br);
            }
            else
            {
                // ausgeschaltete LED, mit einem Hauch des Verlaufs dahinter
                const Col mix = Lerp (FromHex (0x15171b), col, 0.07f);
                br->SetColor (D2D1::ColorF (mix.r, mix.g, mix.b, 1.f));
                rt->FillEllipse (D2D1::Ellipse (D2D1::Point2F (cx, cy), rad, rad), br);
            }
        }
    }

    // Glanzschicht oben auf dem Glas
    const float shineBottom = glassR.top + (glassR.bottom - glassR.top) * 0.42f;
    rt->PushAxisAlignedClip (D2D1::RectF (glassR.left, glassR.top, glassR.right, shineBottom), D2D1_ANTIALIAS_MODE_ALIASED);
    if (ID2D1LinearGradientBrush* g = GradV (rt, glassR.top, shineBottom, 0xffffff, 0.14f, 0xffffff, 0.0f))
    {
        rt->FillRoundedRectangle (glass, g);
        g->Release();
    }
    rt->PopAxisAlignedClip();
}

// Bildschirmschoner-Darstellung: Gerät mittig, Grösse aus den Einstellungen, langsam wandernd
static void DrawScene (Renderer& R, const Settings& S, double t, bool shiftAllowed)
{
    ID2D1HwndRenderTarget* rt = R.rt;
    rt->BeginDraw();
    rt->SetTransform (D2D1::Matrix3x2F::Identity());
    rt->Clear (D2D1::ColorF (0x000000u));

    const D2D1_SIZE_F sz = rt->GetSize();
    const float f = (float) S.size / 100.f;
    const float dw = std::min (sz.width * f, sz.height * f * (kDevW / kDevH));
    const float scale = dw / kDevW;
    const float dh = kDevH * scale;
    float ox = 0.f, oy = 0.f;
    if (S.pixelShift && shiftAllowed)
    {
        // sehr langsame Bewegung (Perioden ca. 10 und 7 Minuten) innerhalb des freien Randes
        ox = (float) std::sin (t * 6.2831853 / 600.0) * (sz.width - dw) * 0.5f * 0.9f;
        oy = (float) std::sin (t * 6.2831853 / 420.0) * (sz.height - dh) * 0.5f * 0.9f;
    }
    DrawDevice (R, S, t, (sz.width - dw) * 0.5f + ox, (sz.height - dh) * 0.5f + oy, scale);

    if (rt->EndDraw() == D2DERR_RECREATE_TARGET)
        R.release();
}

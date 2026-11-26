// ScreenSaver v0.3.2 · Build 7 · 2026-11-26 · Pre-Alpha (Phase 2)
// Windows-Bildschirmschoner (.scr): Carbon-Gerät mit Glasdisplay und LED-Matrix 16 x 64.
// Aufrufe: /s = Bildschirmschoner, /c = Einstellungen, /p <HWND> = Vorschau im Windows-Dialog.
// Technik: Win32 + Direct2D + DirectWrite + WinHTTP, keine weiteren Abhängigkeiten.
#include "Gfx.h"
#include "Ui.h"

// ---------------------------------------------------------------------------------------------
// Bildschirmschoner-Fenster (/s und /p)
// ---------------------------------------------------------------------------------------------
struct SaverWin
{
    Renderer R;
    bool black = false;   // nicht gewählter Monitor: nur schwarz
    bool full  = true;    // Vollbild (false = Vorschau)
};

static Settings  gSettings;
static HWND      gPreviewParent = nullptr;
static ULONGLONG gStartTick = 0;
static POINT     gFirstPos {};
static bool      gHavePos = false;

static void QuitSaver() { PostQuitMessage (0); }

static LRESULT CALLBACK SaverProc (HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    SaverWin* sw = (SaverWin*) GetWindowLongPtrW (h, GWLP_USERDATA);
    const bool inputActive = sw && sw->full && (GetTickCount64() - gStartTick > 700);

    switch (msg)
    {
        case WM_NCCREATE:
            SetWindowLongPtrW (h, GWLP_USERDATA, (LONG_PTR) ((CREATESTRUCTW*) lp)->lpCreateParams);
            return DefWindowProcW (h, msg, wp, lp);

        case WM_CREATE:
            SetTimer (h, 1, 33, nullptr);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            if (! sw || sw->black)
            {
                PAINTSTRUCT ps;
                HDC dc = BeginPaint (h, &ps);
                FillRect (dc, &ps.rcPaint, (HBRUSH) GetStockObject (BLACK_BRUSH));
                EndPaint (h, &ps);
                return 0;
            }
            if (sw->R.ensure (h))
                DrawScene (sw->R, gSettings, (double) GetTickCount64() / 1000.0, sw->full);
            ValidateRect (h, nullptr);
            return 0;
        }

        case WM_TIMER:
            if (sw && ! sw->full && gPreviewParent && ! IsWindow (gPreviewParent))
            {
                QuitSaver();
                return 0;
            }
            if (sw && ! sw->black)
                InvalidateRect (h, nullptr, FALSE);
            return 0;

        case WM_SIZE:
            if (sw && sw->R.rt)
                sw->R.rt->Resize (D2D1::SizeU (LOWORD (lp), HIWORD (lp)));
            return 0;

        case WM_SETCURSOR:
            if (sw && sw->full)
            {
                SetCursor (nullptr);
                return TRUE;
            }
            break;

        case WM_KEYDOWN: case WM_SYSKEYDOWN:
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
            if (inputActive)
                QuitSaver();
            return 0;

        case WM_MOUSEMOVE:
            if (inputActive)
            {
                POINT p;
                GetCursorPos (&p);
                if (! gHavePos)
                {
                    gFirstPos = p;
                    gHavePos = true;
                }
                else if (std::abs (p.x - gFirstPos.x) > 8 || std::abs (p.y - gFirstPos.y) > 8)
                {
                    QuitSaver();
                }
            }
            return 0;

        case WM_ACTIVATEAPP:
            if (wp == FALSE && inputActive)
                QuitSaver();
            return 0;

        case WM_SYSCOMMAND:
            if (sw && sw->full && ((wp & 0xFFF0) == SC_SCREENSAVE || (wp & 0xFFF0) == SC_CLOSE))
                return 0;
            break;

        case WM_DESTROY:
            KillTimer (h, 1);
            if (sw)
                sw->R.release();
            return 0;

        case WM_NCDESTROY:
            delete sw;
            SetWindowLongPtrW (h, GWLP_USERDATA, 0);
            break;
    }
    return DefWindowProcW (h, msg, wp, lp);
}

static bool RegisterSaverClass (HINSTANCE hi)
{
    WNDCLASSW wc {};
    wc.lpfnWndProc = SaverProc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursorW (nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
    wc.lpszClassName = L"HousiScreenSaverWnd";
    return RegisterClassW (&wc) != 0;
}

static int RunMessageLoop()
{
    MSG msg;
    while (GetMessageW (&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage (&msg);
        DispatchMessageW (&msg);
    }
    return 0;
}

static int RunSaver (HINSTANCE hi)
{
    gSettings = LoadSettings();
    gStartTick = GetTickCount64();
    if (! InitGraphics() || ! RegisterSaverClass (hi))
        return 1;

    const auto mons = ListMonitors();
    for (const auto& mon : mons)
    {
        auto* sw = new SaverWin();
        sw->black = ! IsMonitorSelected (gSettings, mon);
        sw->full = true;
        HWND h = CreateWindowExW (WS_EX_TOPMOST, L"HousiScreenSaverWnd", L"ScreenSaver", WS_POPUP | WS_VISIBLE,
                                  mon.rect.left, mon.rect.top, mon.rect.right - mon.rect.left, mon.rect.bottom - mon.rect.top,
                                  nullptr, nullptr, hi, sw);
        (void) h;   // bei Fehler räumt WM_NCDESTROY auf
    }
    return RunMessageLoop();
}

static int RunPreview (HINSTANCE hi, HWND parent)
{
    gSettings = LoadSettings();
    gPreviewParent = parent;
    gStartTick = GetTickCount64();
    if (! IsWindow (parent) || ! InitGraphics() || ! RegisterSaverClass (hi))
        return 1;

    RECT rc {};
    GetClientRect (parent, &rc);
    auto* sw = new SaverWin();
    sw->full = false;
    HWND h = CreateWindowExW (0, L"HousiScreenSaverWnd", L"Vorschau", WS_CHILD | WS_VISIBLE,
                              0, 0, rc.right, rc.bottom, parent, nullptr, hi, sw);
    if (! h)
        return 1;
    return RunMessageLoop();
}

// ---------------------------------------------------------------------------------------------
// Einstieg
// ---------------------------------------------------------------------------------------------
static std::wstring Lower (std::wstring s)
{
    for (auto& c : s)
        c = (wchar_t) towlower (c);
    return s;
}

int WINAPI wWinMain (HINSTANCE hi, HINSTANCE, LPWSTR, int)
{
    SetProcessDPIAware();      // echte Pixel, keine Skalierung durch Windows
    CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED);

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW (GetCommandLineW(), &argc);
    std::wstring a1 = (argv && argc > 1) ? Lower (argv[1]) : L"/c";
    std::wstring a2 = (argv && argc > 2) ? argv[2] : L"";

    int result = 0;
    if (a1.size() >= 2 && (a1[0] == L'/' || a1[0] == L'-'))
    {
        const wchar_t mode = a1[1];
        if (mode == L's')
        {
            result = RunSaver (hi);
        }
        else if (mode == L'p' || mode == L'l')
        {
            std::wstring num = a1.size() > 3 ? a1.substr (3) : a2;   // "/p:1234" oder "/p 1234"
            result = RunPreview (hi, (HWND) (ULONG_PTR) _wtoi64 (num.c_str()));
        }
        else
        {
            result = RunUi (hi);                                      // "/c" oder "/c:1234"
        }
    }
    else
    {
        result = RunUi (hi);
    }

    if (argv)
        LocalFree (argv);
    ReleaseGraphics();
    CoUninitialize();
    return result;
}

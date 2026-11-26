// ScreenSaver v0.3.2 · Build 7 · 2026-11-26 · Pre-Alpha (Phase 2)
// Update-Funktion nach Regelwerk (Grundaufbau): neuestes GitHub-Release, Download mit Fortschritt,
// Prüfsumme SHA-256 gegen die von GitHub gemeldete, Installieren erst danach. Nie automatisch herunterladen.
#pragma once
#include "Common.h"

enum class UpdPhase { Idle, Checking, UpToDate, Available, Error, Downloading, Verifying, Ready };

struct UpdateState
{
    std::atomic<int>      phase { (int) UpdPhase::Idle };
    std::atomic<uint64_t> got { 0 }, total { 0 };
    std::atomic<bool>     cancel { false };
    std::atomic<bool>     busy { false };
    std::mutex            m;
    std::wstring          message, version, url, fileName, localFile;
    std::string           sha256;

    void setMsg (const std::wstring& s) { std::lock_guard<std::mutex> l (m); message = s; }
    std::wstring getMsg() { std::lock_guard<std::mutex> l (m); return message; }
    std::wstring getVersion() { std::lock_guard<std::mutex> l (m); return version; }
};

static UpdateState gUpd;

// --- HTTP (WinHTTP) -----------------------------------------------------------------------------
static bool HttpFetch (const std::wstring& url, const std::function<bool (const BYTE*, DWORD)>& sink,
                       uint64_t* totalOut, DWORD& status, bool& connectFailed)
{
    status = 0;
    connectFailed = false;

    wchar_t host[256] = {}, path[2048] = {}, extra[2048] = {};
    URL_COMPONENTS uc {};
    uc.dwStructSize = sizeof (uc);
    uc.lpszHostName = host;      uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;       uc.dwUrlPathLength = 2048;
    uc.lpszExtraInfo = extra;    uc.dwExtraInfoLength = 2048;
    if (! WinHttpCrackUrl (url.c_str(), 0, 0, &uc))
        return false;

    HINTERNET ses = WinHttpOpen (L"ScreenSaver/" BSL_VERSION, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                 WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (! ses)
    {
        connectFailed = true;
        return false;
    }
    bool ok = false;
    HINTERNET con = WinHttpConnect (ses, host, uc.nPort, 0);
    if (con)
    {
        const std::wstring full = std::wstring (path) + extra;
        HINTERNET req = WinHttpOpenRequest (con, L"GET", full.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
        if (req)
        {
            WinHttpAddRequestHeaders (req, L"Accept: application/vnd.github+json", (DWORD) -1, WINHTTP_ADDREQ_FLAG_ADD);
            if (WinHttpSendRequest (req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
                && WinHttpReceiveResponse (req, nullptr))
            {
                DWORD sz = sizeof (status);
                WinHttpQueryHeaders (req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX);
                if (totalOut)
                {
                    DWORD len = 0;
                    sz = sizeof (len);
                    if (WinHttpQueryHeaders (req, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &len, &sz, WINHTTP_NO_HEADER_INDEX))
                        *totalOut = len;
                }
                ok = true;
                if (status == 200)
                {
                    std::vector<BYTE> buf (64 * 1024);
                    for (;;)
                    {
                        DWORD got = 0;
                        if (! WinHttpReadData (req, buf.data(), (DWORD) buf.size(), &got))
                        {
                            ok = false;
                            break;
                        }
                        if (got == 0)
                            break;
                        if (! sink (buf.data(), got))
                        {
                            ok = false;
                            break;
                        }
                    }
                }
            }
            else
            {
                connectFailed = true;
            }
            WinHttpCloseHandle (req);
        }
        WinHttpCloseHandle (con);
    }
    else
    {
        connectFailed = true;
    }
    WinHttpCloseHandle (ses);
    return ok;
}

// --- Hilfen -------------------------------------------------------------------------------------
static std::string JsonStr (const std::string& j, size_t from, const char* key, size_t* endPos = nullptr)
{
    const std::string k = std::string ("\"") + key + "\"";
    size_t p = j.find (k, from);
    if (p == std::string::npos)
        return {};
    p = j.find (':', p + k.size());
    if (p == std::string::npos)
        return {};
    p = j.find ('"', p);
    if (p == std::string::npos)
        return {};
    const size_t s = p + 1;
    size_t e = s;
    while (e < j.size() && j[e] != '"')
        e += (j[e] == '\\') ? 2 : 1;
    if (endPos)
        *endPos = e;
    return j.substr (s, e - s);
}

static std::wstring Widen (const std::string& s)
{
    return std::wstring (s.begin(), s.end());
}

static std::vector<int> ParseVersion (const std::string& s)
{
    std::vector<int> v;
    int cur = 0;
    bool any = false;
    for (char c : s)
    {
        if (c >= '0' && c <= '9')
        {
            cur = cur * 10 + (c - '0');
            any = true;
        }
        else if (c == '.' && any)
        {
            v.push_back (cur);
            cur = 0;
            any = false;
        }
        else
        {
            break;
        }
    }
    if (any)
        v.push_back (cur);
    return v;
}

static int CompareVersion (std::vector<int> a, std::vector<int> b)
{
    const size_t n = std::max (a.size(), b.size());
    a.resize (n, 0);
    b.resize (n, 0);
    for (size_t i = 0; i < n; ++i)
        if (a[i] != b[i])
            return a[i] < b[i] ? -1 : 1;
    return 0;
}

static std::string Lower8 (std::string s)
{
    for (auto& c : s)
        c = (char) std::tolower ((unsigned char) c);
    return s;
}

// --- Prüfen -------------------------------------------------------------------------------------
static void UpdateCheckThread()
{
    gUpd.phase = (int) UpdPhase::Checking;
    gUpd.setMsg (L"Suche nach Updates ...");

    std::string body;
    DWORD status = 0;
    bool connectFailed = false;
    const std::wstring url = std::wstring (L"https://api.github.com/repos/") + BSL_REPO + L"/releases/latest";
    const bool ok = HttpFetch (url, [&] (const BYTE* d, DWORD n) { body.append ((const char*) d, n); return body.size() < 4 * 1024 * 1024; },
                               nullptr, status, connectFailed);

    if (! ok || status != 200)
    {
        gUpd.phase = (int) UpdPhase::Error;
        if (connectFailed || (! ok && status == 0))
            gUpd.setMsg (L"Keine Verbindung zu GitHub");
        else if (status == 404)
            gUpd.setMsg (L"Noch kein Release auf GitHub");
        else
            gUpd.setMsg (L"GitHub antwortet mit Fehler " + std::to_wstring (status));
        gUpd.busy = false;
        return;
    }

    // Setup-Datei suchen: erstes Asset, dessen Download-Adresse auf .exe endet
    std::string assetUrl, digest;
    size_t pos = 0;
    for (;;)
    {
        size_t endPos = 0;
        const size_t key = body.find ("\"browser_download_url\"", pos);
        if (key == std::string::npos)
            break;
        std::string u = JsonStr (body, key, "browser_download_url", &endPos);
        pos = endPos + 1;
        const std::string lu = Lower8 (u);
        if (lu.size() > 4 && lu.compare (lu.size() - 4, 4, ".exe") == 0)
        {
            assetUrl = u;
            // "digest" steht im Asset-Objekt vor der Download-Adresse
            const size_t d = body.rfind ("\"digest\"", key);
            if (d != std::string::npos && key - d < 2000)
            {
                std::string dv = JsonStr (body, d, "digest");
                if (dv.rfind ("sha256:", 0) == 0)
                    digest = Lower8 (dv.substr (7));
            }
            break;
        }
    }
    if (assetUrl.empty())
    {
        gUpd.phase = (int) UpdPhase::Error;
        gUpd.setMsg (L"Das neueste Release enthält keine Setup-Datei");
        gUpd.busy = false;
        return;
    }

    // Version bevorzugt aus dem Dateinamen (<Produkt>-Setup-vX.Y.Z-BuildN.exe), sonst aus dem Tag
    std::string fileName = assetUrl.substr (assetUrl.find_last_of ('/') + 1);
    std::string verStr;
    const size_t sp = fileName.find ("-Setup-v");
    if (sp != std::string::npos)
        verStr = fileName.substr (sp + 8);
    else
        verStr = JsonStr (body, 0, "tag_name");
    if (! verStr.empty() && (verStr[0] == 'v' || verStr[0] == 'V'))
        verStr.erase (0, 1);
    const std::vector<int> latest = ParseVersion (verStr);
    const std::vector<int> mine = ParseVersion (BSL_VERSION);

    if (latest.empty())
    {
        gUpd.phase = (int) UpdPhase::Error;
        gUpd.setMsg (L"Die Version des Releases ist nicht lesbar");
        gUpd.busy = false;
        return;
    }
    std::wstring shown;
    for (size_t i = 0; i < latest.size(); ++i)
        shown += (i ? L"." : L"") + std::to_wstring (latest[i]);

    if (CompareVersion (latest, mine) <= 0)
    {
        gUpd.phase = (int) UpdPhase::UpToDate;
        gUpd.setMsg (L"Du hast die neueste Version (v" BSL_VERSION L")");
        gUpd.busy = false;
        return;
    }
    {
        std::lock_guard<std::mutex> l (gUpd.m);
        gUpd.version = shown;
        gUpd.url = Widen (assetUrl);
        gUpd.fileName = Widen (fileName);
        gUpd.sha256 = digest;
        gUpd.message = L"Neue Version verfügbar: v" + shown;
    }
    gUpd.phase = (int) UpdPhase::Available;
    gUpd.busy = false;
}

static bool Sha256File (const std::wstring& path, std::string& hex, const std::atomic<bool>& cancel)
{
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider (&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
        return false;
    DWORD objLen = 0, cb = 0;
    BCryptGetProperty (alg, BCRYPT_OBJECT_LENGTH, (PUCHAR) &objLen, sizeof (objLen), &cb, 0);
    std::vector<UCHAR> obj (objLen);
    BCRYPT_HASH_HANDLE h = nullptr;
    bool ok = false;
    if (BCryptCreateHash (alg, &h, obj.data(), objLen, nullptr, 0, 0) == 0)
    {
        HANDLE f = CreateFileW (path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f != INVALID_HANDLE_VALUE)
        {
            std::vector<UCHAR> buf (256 * 1024);
            ok = true;
            for (;;)
            {
                DWORD n = 0;
                if (! ReadFile (f, buf.data(), (DWORD) buf.size(), &n, nullptr))
                {
                    ok = false;
                    break;
                }
                if (n == 0 || cancel)
                    break;
                BCryptHashData (h, buf.data(), n, 0);
            }
            CloseHandle (f);
        }
        UCHAR out[32] = {};
        if (ok && BCryptFinishHash (h, out, 32, 0) == 0)
        {
            static const char* hx = "0123456789abcdef";
            hex.clear();
            for (UCHAR b : out)
            {
                hex += hx[b >> 4];
                hex += hx[b & 15];
            }
        }
        else
        {
            ok = false;
        }
        BCryptDestroyHash (h);
    }
    BCryptCloseAlgorithmProvider (alg, 0);
    return ok;
}

// --- Herunterladen ------------------------------------------------------------------------------
static void UpdateDownloadThread()
{
    std::wstring url, fileName;
    std::string sha;
    {
        std::lock_guard<std::mutex> l (gUpd.m);
        url = gUpd.url;
        fileName = gUpd.fileName;
        sha = gUpd.sha256;
    }
    wchar_t tmp[MAX_PATH] = {};
    GetTempPathW (MAX_PATH, tmp);
    const std::wstring local = std::wstring (tmp) + fileName;

    gUpd.got = 0;
    gUpd.total = 0;
    gUpd.cancel = false;
    gUpd.phase = (int) UpdPhase::Downloading;
    gUpd.setMsg (L"Lade herunter ...");

    HANDLE f = CreateFileW (local.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE)
    {
        gUpd.phase = (int) UpdPhase::Error;
        gUpd.setMsg (L"Die Datei kann nicht gespeichert werden");
        gUpd.busy = false;
        return;
    }
    uint64_t total = 0;
    DWORD status = 0;
    bool connectFailed = false;
    bool writeOk = true;
    const bool ok = HttpFetch (url, [&] (const BYTE* d, DWORD n)
    {
        if (gUpd.cancel)
            return false;
        DWORD w = 0;
        if (! WriteFile (f, d, n, &w, nullptr) || w != n)
        {
            writeOk = false;
            return false;
        }
        gUpd.got += n;
        return true;
    }, &total, status, connectFailed);
    CloseHandle (f);
    (void) total;

    if (gUpd.cancel)
    {
        DeleteFileW (local.c_str());
        gUpd.phase = (int) UpdPhase::Available;
        gUpd.setMsg (L"Download abgebrochen");
        gUpd.busy = false;
        return;
    }
    if (! ok || status != 200 || ! writeOk)
    {
        DeleteFileW (local.c_str());
        gUpd.phase = (int) UpdPhase::Error;
        gUpd.setMsg (connectFailed ? L"Keine Verbindung zu GitHub" : L"Download fehlgeschlagen");
        gUpd.busy = false;
        return;
    }

    gUpd.phase = (int) UpdPhase::Verifying;
    gUpd.setMsg (L"Prüfe Prüfsumme ...");
    std::string hex;
    if (sha.empty())
    {
        DeleteFileW (local.c_str());
        gUpd.phase = (int) UpdPhase::Error;
        gUpd.setMsg (L"GitHub meldet keine Prüfsumme, die Datei wird verworfen");
        gUpd.busy = false;
        return;
    }
    if (! Sha256File (local, hex, gUpd.cancel) || hex != sha)
    {
        DeleteFileW (local.c_str());
        gUpd.phase = (int) UpdPhase::Error;
        gUpd.setMsg (L"Prüfsumme stimmt nicht, die Datei wurde verworfen");
        gUpd.busy = false;
        return;
    }
    {
        std::lock_guard<std::mutex> l (gUpd.m);
        gUpd.localFile = local;
        gUpd.message = L"Download fertig, Prüfsumme in Ordnung";
    }
    gUpd.phase = (int) UpdPhase::Ready;
    gUpd.busy = false;
}

static void StartUpdateCheck()
{
    bool expected = false;
    if (gUpd.busy.compare_exchange_strong (expected, true))
        std::thread (UpdateCheckThread).detach();
}

static void StartUpdateDownload()
{
    bool expected = false;
    if (gUpd.busy.compare_exchange_strong (expected, true))
        std::thread (UpdateDownloadThread).detach();
}

static void InstallUpdate()
{
    std::wstring f;
    {
        std::lock_guard<std::mutex> l (gUpd.m);
        f = gUpd.localFile;
    }
    if (! f.empty())
        ShellExecuteW (nullptr, L"open", f.c_str(), nullptr, nullptr, SW_SHOWNORMAL);   // der Installer fragt selbst nach Adminrechten
}

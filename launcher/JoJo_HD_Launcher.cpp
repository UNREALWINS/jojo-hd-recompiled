#include <windows.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <objidl.h>
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <system_error>

namespace fs = std::filesystem;
using namespace Gdiplus;

static const wchar_t* kClassName = L"JoJoHDLauncherFinalUI";
static const wchar_t* kTitle = L"JoJo's Bizarre Adventure HD Ver. - Recompiled Edition - v1.0 Beta";

struct Settings {
    std::wstring resolution = L"1280x720";
    bool fullscreen = true;
    bool vsync = true;
};

static Settings gSettings;
static HWND gMain = nullptr;
static ULONG_PTR gGdiToken = 0;
static std::unique_ptr<Image> gBg;
static IStream* gBgStream = nullptr;
static std::wstring gIni;
static RECT gCloseRect{1000, 0, 1031, 28};
static RECT gMinRect{910, 0, 948, 28};
static RECT gResolutionRect{422, 347, 633, 383};
static RECT gFullscreenRect{412, 386, 628, 414};
static RECT gVsyncRect{412, 419, 628, 445};
static RECT gSavesRect{823, 344, 1016, 390};
static RECT gLogsRect{823, 395, 1016, 440};
static RECT gAdvancedRect{823, 446, 1016, 491};
static RECT gPlayRect{249, 503, 706, 575};
static RECT gDiagRect{725, 505, 1016, 575};
static RECT gCreditsRect{249, 590, 468, 637};
static RECT gVerifyRect{477, 590, 690, 637};
static RECT gExitRect{702, 590, 1016, 637};
static int gHover = 0;

static std::wstring ExeDir() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path().wstring();
}

static bool Exists(const fs::path& p) {
    std::error_code ec;
    return fs::exists(p, ec);
}

static std::wstring Quote(const std::wstring& s) { return L"\"" + s + L"\""; }

static std::wstring FindGameExe() {
    fs::path root = ExeDir();
    const fs::path candidates[] = {
        root / L"jojohd.exe",
        root / L"out" / L"build" / L"win-amd64-release" / L"jojohd.exe"
    };
    for (auto& p : candidates) if (Exists(p)) return p.wstring();
    return L"";
}

static std::wstring FindGameData() {
    fs::path p = fs::path(ExeDir()) / L"extracted";
    if (Exists(p / L"default.xex")) return p.wstring();
    return L"";
}

static std::wstring HelperExe() {
    return (fs::path(ExeDir()) / L"JoJo HD Helper.exe").wstring();
}

static std::wstring LogsDir() {
    return (fs::path(ExeDir()) / L"logs").wstring();
}

static std::wstring SavesDir() {
    wchar_t user[MAX_PATH]{};
    DWORD n = GetEnvironmentVariableW(L"USERPROFILE", user, MAX_PATH);
    fs::path base = n ? fs::path(user) : fs::path(ExeDir());
    return (base / L"Documents" / L"jojohd").wstring();
}

static bool PtIn(const RECT& r, int x, int y) {
    POINT p{x, y};
    return PtInRect(&r, p) != FALSE;
}

static int MouseX(LPARAM lp) { return (int)(short)LOWORD((DWORD_PTR)lp); }
static int MouseY(LPARAM lp) { return (int)(short)HIWORD((DWORD_PTR)lp); }

static void EnsureDir(const std::wstring& p) {
    std::error_code ec;
    fs::create_directories(fs::path(p), ec);
}

static void OpenFolder(const std::wstring& p) {
    EnsureDir(p);
    ShellExecuteW(gMain, L"open", p.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

static void LoadSettings() {
    gIni = (fs::path(ExeDir()) / L"JoJo_HD_Launcher.ini").wstring();
    wchar_t buf[64]{};
    GetPrivateProfileStringW(L"Launcher", L"Resolution", L"1280x720", buf, 64, gIni.c_str());
    gSettings.resolution = buf;
    gSettings.fullscreen = GetPrivateProfileIntW(L"Launcher", L"Fullscreen", 1, gIni.c_str()) != 0;
    gSettings.vsync = GetPrivateProfileIntW(L"Launcher", L"VSync", 1, gIni.c_str()) != 0;
}

static void SaveSettings() {
    WritePrivateProfileStringW(L"Launcher", L"Resolution", gSettings.resolution.c_str(), gIni.c_str());
    WritePrivateProfileStringW(L"Launcher", L"Fullscreen", gSettings.fullscreen ? L"1" : L"0", gIni.c_str());
    WritePrivateProfileStringW(L"Launcher", L"VSync", gSettings.vsync ? L"1" : L"0", gIni.c_str());
}

static void DrawTextOverlay(HDC hdc, const wchar_t* text, RECT r, int size, COLORREF color, bool bold=false, UINT align=DT_LEFT) {
    HFONT f = CreateFontW(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(hdc, f);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    DrawTextW(hdc, text, -1, &r, align | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
    DeleteObject(f);
}

static bool LoadEmbeddedBackground(HINSTANCE hInst) {
    HRSRC hrsrc = FindResourceW(hInst, MAKEINTRESOURCEW(101), RT_RCDATA);
    if (!hrsrc) return false;
    HGLOBAL hres = LoadResource(hInst, hrsrc);
    if (!hres) return false;
    DWORD size = SizeofResource(hInst, hrsrc);
    const void* data = LockResource(hres);
    if (!data || !size) return false;

    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hmem) return false;
    void* dest = GlobalLock(hmem);
    if (!dest) {
        GlobalFree(hmem);
        return false;
    }
    CopyMemory(dest, data, size);
    GlobalUnlock(hmem);

    IStream* stream = nullptr;
    if (CreateStreamOnHGlobal(hmem, TRUE, &stream) != S_OK || !stream) {
        GlobalFree(hmem);
        return false;
    }

    Image* img = Image::FromStream(stream, FALSE);
    if (!img || img->GetLastStatus() != Ok) {
        if (img) delete img;
        stream->Release();
        return false;
    }

    gBg.reset(img);
    gBgStream = stream;
    return true;
}

static void DrawState(HDC hdc) {
    RECT rr{439, 348, 616, 380};
    HBRUSH rb = CreateSolidBrush(RGB(42, 48, 61));
    FillRect(hdc, &rr, rb);
    DeleteObject(rb);
    DrawTextOverlay(hdc, gSettings.resolution.c_str(), RECT{455,348,598,380}, 17, RGB(238,238,238));
    DrawTextOverlay(hdc, L"v", RECT{598,348,617,380}, 16, RGB(210,210,210), false, DT_CENTER);

    auto checkbox = [&](int x, int y, bool checked) {
        RECT box{x,y,x+23,y+23};
        HBRUSH fill = CreateSolidBrush(RGB(28,31,40));
        FillRect(hdc, &box, fill); DeleteObject(fill);
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(220,220,220));
        HGDIOBJ oldp = SelectObject(hdc, pen);
        HGDIOBJ oldb = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, box.left, box.top, box.right, box.bottom);
        if (checked) {
            HPEN cp = CreatePen(PS_SOLID, 3, RGB(240,240,240));
            SelectObject(hdc, cp);
            MoveToEx(hdc, x+5, y+12, nullptr); LineTo(hdc, x+10, y+17); LineTo(hdc, x+19, y+6);
            DeleteObject(cp);
        }
        SelectObject(hdc, oldb); SelectObject(hdc, oldp); DeleteObject(pen);
    };
    checkbox(416, 388, gSettings.fullscreen);
    checkbox(416, 421, gSettings.vsync);

    if (gHover != 0) {
        RECT r{};
        switch (gHover) {
            case 1:r=gPlayRect;break; case 2:r=gDiagRect;break; case 3:r=gSavesRect;break;
            case 4:r=gLogsRect;break; case 5:r=gAdvancedRect;break; case 6:r=gCreditsRect;break;
            case 7:r=gVerifyRect;break; case 8:r=gExitRect;break; default:return;
        }
        HPEN p=CreatePen(PS_SOLID,3,RGB(255,235,135));
        HGDIOBJ op=SelectObject(hdc,p); HGDIOBJ ob=SelectObject(hdc,GetStockObject(NULL_BRUSH));
        Rectangle(hdc,r.left+2,r.top+2,r.right-2,r.bottom-2);
        SelectObject(hdc,ob);SelectObject(hdc,op);DeleteObject(p);
    }
}

static void ChooseResolution(HWND hwnd) {
    HMENU menu = CreatePopupMenu();
    const wchar_t* opts[] = {L"720p",L"900p",L"1080p",L"1440p",L"4k",L"1280x720",L"1366x768",L"1600x900",L"1920x1080",L"2560x1440",L"3840x2160"};
    for (int i=0;i<11;i++) AppendMenuW(menu, MF_STRING, 2000+i, opts[i]);
    POINT pt{gResolutionRect.left, gResolutionRect.bottom};
    ClientToScreen(hwnd,&pt);
    UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD|TPM_LEFTALIGN|TPM_TOPALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(menu);
    if (cmd>=2000 && cmd<2011) {
        gSettings.resolution = opts[cmd-2000];
        SaveSettings(); InvalidateRect(hwnd,nullptr,FALSE);
    }
}

static bool VerifyFiles(HWND hwnd) {
    std::wstring game=FindGameExe(), data=FindGameData(), helper=HelperExe();
    std::wstring msg;
    msg += game.empty()?L"[ERRO] jojohd.exe nao encontrado\n":L"[OK] jojohd.exe\n";
    msg += data.empty()?L"[ERRO] extracted\\default.xex nao encontrado\n":L"[OK] extracted\\default.xex\n";
    msg += Exists(helper)?L"[OK] JoJo HD Helper.exe\n":L"[ERRO] JoJo HD Helper.exe nao encontrado\n";
    bool ok=!game.empty()&&!data.empty()&&Exists(helper);
    MessageBoxW(hwnd,msg.c_str(),L"Verificar arquivos",(ok?MB_ICONINFORMATION:MB_ICONWARNING)|MB_OK);
    return ok;
}

static bool Launch(bool diagnostic) {
    std::wstring game=FindGameExe(), data=FindGameData();
    if (game.empty() || data.empty()) { VerifyFiles(gMain); return false; }

    std::wstring cmd = Quote(game) + L" --game_data_root " + Quote(data) + L" --gpu_plugin xenos --license_mask=1";
    if (!gSettings.resolution.empty()) cmd += L" --resolution " + gSettings.resolution;
    cmd += gSettings.fullscreen ? L" --fullscreen" : L" --no-fullscreen";
    if (!gSettings.vsync) cmd += L" --d3d12_allow_variable_refresh_rate_and_tearing";
    if (diagnostic) {
        EnsureDir(LogsDir());
        cmd += L" --log_level trace --log_file " + Quote((fs::path(LogsDir())/L"jojo_diagnostic.log").wstring()) + L" --log_flush_interval 100";
    }

    std::vector<wchar_t> mutableCmd(cmd.begin(),cmd.end()); mutableCmd.push_back(L'\0');
    STARTUPINFOW si{}; si.cb=sizeof(si); PROCESS_INFORMATION pi{};
    std::wstring work=fs::path(game).parent_path().wstring();
    BOOL ok=CreateProcessW(game.c_str(), mutableCmd.data(), nullptr,nullptr,FALSE,CREATE_UNICODE_ENVIRONMENT,nullptr,work.c_str(),&si,&pi);
    if(!ok){MessageBoxW(gMain,L"Falha ao iniciar o jogo.",L"JoJo HD",MB_ICONERROR|MB_OK);return false;}
    CloseHandle(pi.hThread);

    std::wstring helper=HelperExe();
    if(Exists(helper)){
        std::wstring hc=Quote(helper)+L" --pid "+std::to_wstring(pi.dwProcessId);
        std::vector<wchar_t> hcm(hc.begin(),hc.end());hcm.push_back(L'\0');
        STARTUPINFOW hsi{};hsi.cb=sizeof(hsi);PROCESS_INFORMATION hpi{};
        if(CreateProcessW(helper.c_str(),hcm.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW|CREATE_UNICODE_ENVIRONMENT,nullptr,ExeDir().c_str(),&hsi,&hpi)){
            CloseHandle(hpi.hThread);CloseHandle(hpi.hProcess);
        }
    }
    CloseHandle(pi.hProcess);
    SaveSettings();
    DestroyWindow(gMain);
    return true;
}

static int HoverFromPoint(int x,int y){
    if(PtIn(gPlayRect,x,y))return 1;if(PtIn(gDiagRect,x,y))return 2;if(PtIn(gSavesRect,x,y))return 3;
    if(PtIn(gLogsRect,x,y))return 4;if(PtIn(gAdvancedRect,x,y))return 5;if(PtIn(gCreditsRect,x,y))return 6;
    if(PtIn(gVerifyRect,x,y))return 7;if(PtIn(gExitRect,x,y))return 8;return 0;
}

static LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_CREATE:gMain=hwnd;LoadSettings();return 0;
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{
        PAINTSTRUCT ps{};HDC hdc=BeginPaint(hwnd,&ps);Graphics g(hdc);
        if(gBg&&gBg->GetLastStatus()==Ok){g.SetInterpolationMode(InterpolationModeHighQualityBicubic);g.DrawImage(gBg.get(),0,0,1031,648);}
        else{HBRUSH b=CreateSolidBrush(RGB(20,15,28));RECT r{0,0,1031,648};FillRect(hdc,&r,b);DeleteObject(b);}
        DrawState(hdc);EndPaint(hwnd,&ps);return 0;}
    case WM_MOUSEMOVE:{int x=MouseX(lp),y=MouseY(lp);int h=HoverFromPoint(x,y);if(h!=gHover){gHover=h;InvalidateRect(hwnd,nullptr,FALSE);}TRACKMOUSEEVENT t{sizeof(t),TME_LEAVE,hwnd,0};TrackMouseEvent(&t);return 0;}
    case WM_MOUSELEAVE:gHover=0;InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_LBUTTONDOWN:{int x=MouseX(lp),y=MouseY(lp);
        if(PtIn(gCloseRect,x,y)||PtIn(gExitRect,x,y)){DestroyWindow(hwnd);return 0;}
        if(PtIn(gMinRect,x,y)){ShowWindow(hwnd,SW_MINIMIZE);return 0;}
        if(y<30){ReleaseCapture();SendMessageW(hwnd,WM_NCLBUTTONDOWN,HTCAPTION,0);return 0;}
        if(PtIn(gResolutionRect,x,y)){ChooseResolution(hwnd);return 0;}
        if(PtIn(gFullscreenRect,x,y)){gSettings.fullscreen=!gSettings.fullscreen;SaveSettings();InvalidateRect(hwnd,nullptr,FALSE);return 0;}
        if(PtIn(gVsyncRect,x,y)){gSettings.vsync=!gSettings.vsync;SaveSettings();InvalidateRect(hwnd,nullptr,FALSE);return 0;}
        if(PtIn(gSavesRect,x,y)){OpenFolder(SavesDir());return 0;}
        if(PtIn(gLogsRect,x,y)){OpenFolder(LogsDir());return 0;}
        if(PtIn(gAdvancedRect,x,y)){MessageBoxW(hwnd,L"Opcoes avancadas\n\nGPU: xenos (recomendado)\nF12: gerenciado pelo JoJo HD Helper\nCursor: ocultado automaticamente durante o jogo",L"Opcoes avancadas",MB_ICONINFORMATION|MB_OK);return 0;}
        if(PtIn(gCreditsRect,x,y)){MessageBoxW(hwnd,L"JoJo's Bizarre Adventure HD Ver.\nRecompiled Edition - v1.0 Beta\n\nLauncher criado para a build recompilada com ReXGlue.",L"Creditos",MB_ICONINFORMATION|MB_OK);return 0;}
        if(PtIn(gVerifyRect,x,y)){VerifyFiles(hwnd);return 0;}
        if(PtIn(gPlayRect,x,y)){Launch(false);return 0;}
        if(PtIn(gDiagRect,x,y)){Launch(true);return 0;}
        return 0;}
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int){
    SetProcessDPIAware();
    GdiplusStartupInput gi;GdiplusStartup(&gGdiToken,&gi,nullptr);
    LoadEmbeddedBackground(h);
    LoadSettings();
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=WndProc;wc.hInstance=h;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hIcon=LoadIcon(h,MAKEINTRESOURCE(1));if(!wc.hIcon)wc.hIcon=LoadIcon(nullptr,IDI_APPLICATION);wc.hIconSm=wc.hIcon;wc.lpszClassName=kClassName;RegisterClassExW(&wc);
    RECT wa{};SystemParametersInfoW(SPI_GETWORKAREA,0,&wa,0);int x=wa.left+((wa.right-wa.left)-1031)/2;int y=wa.top+((wa.bottom-wa.top)-648)/2;
    HWND hwnd=CreateWindowExW(WS_EX_APPWINDOW,kClassName,kTitle,WS_POPUP,x,y,1031,648,nullptr,nullptr,h,nullptr);if(!hwnd)return 1;
    ShowWindow(hwnd,SW_SHOW);UpdateWindow(hwnd);
    MSG m{};while(GetMessageW(&m,nullptr,0,0)>0){TranslateMessage(&m);DispatchMessageW(&m);}gBg.reset();if(gBgStream){gBgStream->Release();gBgStream=nullptr;}if(gGdiToken)GdiplusShutdown(gGdiToken);return(int)m.wParam;
}

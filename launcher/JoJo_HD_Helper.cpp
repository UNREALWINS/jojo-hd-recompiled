#include <windows.h>
#include <string>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static DWORD gPid = 0;
static HANDLE gProcess = nullptr;
static bool gShowingDialog = false;

static std::wstring ExeDir() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path().wstring();
}

static void LogLine(const std::wstring& text) {
    std::error_code ec;
    fs::path dir = fs::path(ExeDir()) / L"logs";
    fs::create_directories(dir, ec);
    std::wofstream f(dir / L"jojo_helper.log", std::ios::app);
    if (!f) return;
    SYSTEMTIME st{};
    GetLocalTime(&st);
    f << L"[" << st.wHour << L":" << st.wMinute << L":" << st.wSecond << L"] " << text << L"\n";
}

static DWORD ParsePid(const wchar_t* cmd) {
    std::wstring s = cmd ? cmd : L"";
    size_t p = s.find(L"--pid");
    if (p == std::wstring::npos) return 0;
    p += 5;
    while (p < s.size() && (s[p] == L' ' || s[p] == L'=')) ++p;
    return wcstoul(s.c_str() + p, nullptr, 10);
}

static BOOL CALLBACK CloseProc(HWND hwnd, LPARAM) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == gPid) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
    }
    return TRUE;
}

static void MoveCursorToCorner() {
    if (gShowingDialog) return;
    POINT pt{};
    HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(mon, &mi)) {
        SetCursorPos(mi.rcMonitor.right - 2, mi.rcMonitor.bottom - 2);
    } else {
        SetCursorPos(GetSystemMetrics(SM_CXSCREEN) - 2, GetSystemMetrics(SM_CYSCREEN) - 2);
    }
}

static void CloseGame() {
    if (!gProcess) return;
    LogLine(L"F12 confirmado: solicitando fechamento do jogo.");
    EnumWindows(CloseProc, 0);
    if (WaitForSingleObject(gProcess, 1800) == WAIT_TIMEOUT) {
        LogLine(L"Fechamento normal excedeu timeout; usando TerminateProcess.");
        TerminateProcess(gProcess, 0);
        WaitForSingleObject(gProcess, 1800);
    }
}

static bool F12IsDown() {
    return (GetAsyncKeyState(VK_F12) & 0x8000) != 0;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR cmd, int) {
    gPid = ParsePid(cmd);
    if (!gPid) {
        LogLine(L"ERRO: PID ausente.");
        return 2;
    }

    gProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, gPid);
    if (!gProcess) {
        LogLine(L"ERRO: OpenProcess falhou para PID " + std::to_wstring(gPid) + L". Codigo=" + std::to_wstring(GetLastError()));
        return 3;
    }

    LogLine(L"Helper iniciado para PID " + std::to_wstring(gPid) + L".");

    Sleep(500);
    MoveCursorToCorner();

    bool prevF12 = false;
    ULONGLONG lastCursor = 0;

    for (;;) {
        if (WaitForSingleObject(gProcess, 0) == WAIT_OBJECT_0) {
            LogLine(L"Jogo encerrado; finalizando Helper.");
            break;
        }

        bool down = F12IsDown();
        if (down && !prevF12) {
            LogLine(L"F12 detectado.");
            gShowingDialog = true;
            ShowCursor(TRUE);
            int r = MessageBoxW(
                nullptr,
                L"Deseja sair do jogo?",
                L"JoJo HD Recompiled",
                MB_YESNO | MB_ICONQUESTION | MB_TOPMOST | MB_SETFOREGROUND | MB_SYSTEMMODAL);
            gShowingDialog = false;

            if (r == IDYES) {
                CloseGame();
                break;
            }
            MoveCursorToCorner();
        }
        prevF12 = down;

        ULONGLONG now = GetTickCount64();
        if (!gShowingDialog && now - lastCursor >= 750) {
            MoveCursorToCorner();
            lastCursor = now;
        }

        Sleep(15);
    }

    CloseHandle(gProcess);
    gProcess = nullptr;
    return 0;
}

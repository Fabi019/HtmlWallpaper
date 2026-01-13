#include "HtmlWallpaper.h"
#include "Wallpaper.h"

#define WM_NMICON WM_USER+1

const TCHAR szTitle[] = _T("HtmlWallpaper");
const TCHAR szSettingsFile[] = _T("settings.ini");
const TCHAR szCategory[] = _T("Settings");

static UINT s_uTaskbarRestart;
static NOTIFYICONDATA s_nid;
static HANDLE s_mutex = NULL;
static HICON s_icon = NULL;
static HICON s_iconDisabled = NULL;

static TCHAR s_settingsFile[MAX_PATH];
static BOOL s_disabled;
static TCHAR s_url[MAX_PATH];
static BOOL s_efficiencyMode;
static BOOL s_autoRun;

static Wallpaper* s_wallpaper;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Check if another instance is already running
    s_mutex = CreateMutex(NULL, TRUE, szTitle);

    if (s_mutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL,
            _T("Another instance of the application is already running."),
            szTitle,
            MB_ICONINFORMATION);
        CloseHandle(s_mutex);
        return 0; // Exit the second instance
    }

    // Load settings
    InitSettings(false);
    SetStartup(s_autoRun);
    SetEfficiencyMode(s_efficiencyMode);

    // Initialize variables
    WNDCLASSEX wcex = {
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        WndProc, 0L, 0L,
        GetModuleHandle(NULL),
        NULL, NULL, NULL, NULL,
        szTitle, NULL
    };

    // Register window class
    if (!RegisterClassEx(&wcex)) {
        MessageBox(NULL, _T("Call to RegisterClassEx failed!"), szTitle, MB_ICONERROR);
        return 1;
    }

    // Create a hidden window
    HWND hWnd = CreateWindow(
        wcex.lpszClassName, szTitle,
        WS_OVERLAPPEDWINDOW,
        0, 0, 0, 0, NULL, NULL,
        wcex.hInstance, NULL
    );

    if (!hWnd) {
        MessageBox(NULL, _T("Call to CreateWindowEx failed!"), szTitle, MB_ICONERROR);
        return 1;
    }

    Wallpaper wallpaper{s_url};
	wallpaper.Initialize(wcex.lpszClassName, hInstance);
    s_wallpaper = &wallpaper;

    s_icon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON));
    s_iconDisabled = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));

    // Initialize NOTIFYICONDATA structure
    s_nid.cbSize = sizeof(NOTIFYICONDATA);
    s_nid.hWnd = hWnd;
    s_nid.uID = 1;
    s_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    s_nid.uCallbackMessage = WM_NMICON;
    s_nid.hIcon = s_icon;
    _tcscpy_s(s_nid.szTip, szTitle);

    // Add the tray icon
    Shell_NotifyIcon(NIM_ADD, &s_nid);

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up and remove the tray icon
    Shell_NotifyIcon(NIM_DELETE, &s_nid);
    UnregisterClass(wcex.lpszClassName, wcex.hInstance);

    // Release the mutex before exiting
    if (s_mutex)
    {
        ReleaseMutex(s_mutex);
        CloseHandle(s_mutex);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(
    _In_ HWND   hWnd,
    _In_ UINT   message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
) {
    switch (message) {
    case WM_CREATE:
        s_uTaskbarRestart = RegisterWindowMessage(_T("TaskbarCreated"));
        break;

        // Tray icon message
    case WM_NMICON:
        if (LOWORD(lParam) == WM_RBUTTONUP) OpenPopup(hWnd);
        else if (LOWORD(lParam) == WM_LBUTTONUP) Toggle();
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        if (s_wallpaper) {
            s_wallpaper->Resize();
        }
        break;

    default:
        if (message == s_uTaskbarRestart)
        {
            Shell_NotifyIcon(NIM_ADD, &s_nid);
            break;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

VOID OpenPopup(HWND hWnd)
{
    // Initialize Popup menu
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 4, s_disabled ? _T("Enable") : _T("Disable"));
    AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
    AppendMenu(hMenu, MF_STRING, 2, _T("Settings"));
    AppendMenu(hMenu, MF_STRING, 3, _T("About..."));
    AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
    AppendMenu(hMenu, MF_STRING, 1, _T("Exit"));

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);

    UINT cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
    switch (cmd) {
    case 1: // Exit
        PostMessage(hWnd, WM_CLOSE, 0, 0);
        break;

    case 2: { // Settings
        DWORD fileAttributes = GetFileAttributes(s_settingsFile);
        if ((fileAttributes == INVALID_FILE_ATTRIBUTES)
            || (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            int result = MessageBox(NULL,
                _T("The settings file does not exist. Do you want to create it?"),
                szTitle,
                MB_YESNO | MB_ICONQUESTION);
            if (result == IDYES) {
                InitSettings(true);
            }
            else {
                break;
            }
        }
        ShellExecute(NULL, _T("open"), s_settingsFile, NULL, NULL, SW_SHOWNORMAL);
        break;
    }
    case 3: // About
        ShellExecute(NULL, _T("open"), _T("https://www.github.com/Fabi019/HtmlWallpaper"), NULL, NULL, SW_SHOWNORMAL);
        break;

    case 4: // Toggle
        Toggle();
        break;
    }

    DestroyMenu(hMenu);
}

VOID InitSettings(BOOL createNew) {
    GetModuleFileName(NULL, s_settingsFile, MAX_PATH);
    _tcsncpy_s(_tcsrchr(s_settingsFile, _T('\\')) + 1, MAX_PATH, szSettingsFile, MAX_PATH - _tcslen(s_settingsFile));

    if (createNew)
    {
        WritePrivateProfileString(szCategory, _T("Url"), _T("https://flux.sandydoo.me"), s_settingsFile);
        WritePrivateProfileString(szCategory, _T("AutoStart"), _T("0"), s_settingsFile);
        WritePrivateProfileString(szCategory, _T("EfficiencyMode"), _T("1"), s_settingsFile);
        return;
    }

    TCHAR szValue[MAX_PATH];
    GetPrivateProfileString(szCategory, _T("Url"), _T("https://flux.sandydoo.me"), szValue, MAX_PATH, s_settingsFile);
	_tcscpy_s(s_url, MAX_PATH, szValue);
    GetPrivateProfileString(szCategory, _T("AutoStart"), _T("0"), szValue, 2, s_settingsFile);
    s_autoRun = !!_tstoi(szValue);
    GetPrivateProfileString(szCategory, _T("EfficiencyMode"), _T("1"), szValue, 2, s_settingsFile);
    s_efficiencyMode = !!_tstoi(szValue);
}

VOID SetEfficiencyMode(BOOL enable) {
    if (!enable) return;

    // Set process priority to idle
    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

    // Throttle execution speed (EcoQos)
    PROCESS_POWER_THROTTLING_STATE pic;
    pic.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    pic.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    pic.StateMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;

    typedef BOOL(WINAPI* PFN_SetProcessInformation)(
        HANDLE hProcess,
        PROCESS_INFORMATION_CLASS ProcessInformationClass,
        LPVOID ProcessInformation,
        DWORD ProcessInformationSize
        );

    // Try retrieving the function
    auto pSetProcessInformation =
        (PFN_SetProcessInformation)GetProcAddress(
            GetModuleHandleA("kernel32.dll"), "SetProcessInformation");

    if (pSetProcessInformation)
    {
        pSetProcessInformation(GetCurrentProcess(),
            ProcessPowerThrottling,
            &pic,
            sizeof(PROCESS_POWER_THROTTLING_STATE));
    }
}

VOID SetStartup(BOOL enable)
{
    HKEY hKey;
    RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_SET_VALUE, &hKey);

    if (enable) {
        TCHAR szPath[MAX_PATH];
        GetModuleFileName(NULL, szPath, MAX_PATH);
        RegSetValueEx(hKey, szTitle, 0, REG_SZ, (BYTE*)szPath, (DWORD)(_tcslen(szPath) + 1) * sizeof(TCHAR));
    }
    else {
        RegDeleteValue(hKey, szTitle);
    }

    RegCloseKey(hKey);
}

VOID Toggle()
{
    s_disabled = !s_disabled;
    s_nid.hIcon = s_disabled ? s_iconDisabled : s_icon;
    Shell_NotifyIcon(NIM_MODIFY, &s_nid);
    if (s_wallpaper) {
        s_wallpaper->Disable(s_disabled);
	}
}
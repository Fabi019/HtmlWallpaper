#include "Wallpaper.h"

using namespace Microsoft::WRL;

Wallpaper::Wallpaper(const wchar_t* initial_url)
    : m_url(initial_url), m_workerW(nullptr), m_window(nullptr), m_hInstance(nullptr) {}

Wallpaper::~Wallpaper() {
    if (m_controller) m_controller->Close();
	if (m_window) DestroyWindow(m_window);
    CoUninitialize();
}

bool Wallpaper::Initialize(LPCWSTR className, HINSTANCE hInstance) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"COM init failed", L"Error", MB_ICONERROR);
        return false;
    }

    m_hInstance = hInstance;
    m_workerW = FindWorkerW();
    if (!m_workerW) return false;
    return CreateWallpaperWindow(className);
}

HWND Wallpaper::FindWorkerW() {
    HWND progman = FindWindow(L"Progman", nullptr);

    // Tell Progman to spawn WorkerW
    SendMessageTimeout(
        progman,
        0x052C,
        0xD,
        0x1,
        SMTO_NORMAL,
        100,
        nullptr
    );

    HWND workerW = nullptr;
    workerW = FindWindowEx(progman, nullptr, L"WorkerW", nullptr);
    return workerW;
}

bool Wallpaper::CreateWallpaperWindow(LPCWSTR className) {
    RECT rc;
    GetClientRect(GetDesktopWindow(), &rc);

    m_window = CreateWindowEx(
        WS_EX_NOACTIVATE,
        className,
        L"",
        WS_CHILD | WS_VISIBLE,
        0, 0,
        rc.right,
        rc.bottom,
        m_workerW,
        nullptr,
        m_hInstance,
        nullptr
    );

    if (!m_window) return false;

    InitWebView();
    return true;
}

void Wallpaper::InitWebView() {
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        nullptr,
        nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT hr, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(hr) || !env) {
                    MessageBox(nullptr, L"WebView2 environment creation failed", L"Error", MB_ICONERROR);
                    return hr;
                }

                env->CreateCoreWebView2Controller(
                    m_window,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT hr, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(hr) || !controller) {
                                MessageBox(nullptr, L"WebView2 controller creation failed", L"Error", MB_ICONERROR);
                                return hr;
                            }

                            m_controller = controller;
                            Resize();

                            controller->get_CoreWebView2(&m_webview);
                            m_webview->Navigate(m_url);
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
}

bool Wallpaper::Resize() {
    if (!m_controller || !m_window) return false;
    RECT rc;
    GetClientRect(m_window, &rc);
    m_controller->put_Bounds(rc);
    return true;
}

bool Wallpaper::LoadURL(const wchar_t* url) {
    if (!m_webview) return false;
    m_webview->Navigate(url);
    return true;
}

void Wallpaper::Disable(bool disable) const {
    if (!m_window) return;
    ShowWindow(m_window, disable ? SW_HIDE : SW_SHOW);
}

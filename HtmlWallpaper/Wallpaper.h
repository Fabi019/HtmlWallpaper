#pragma once

#include "stdafx.h"

#include <wrl.h>
#include <WebView2.h>

class Wallpaper {
public:
    Wallpaper(const wchar_t* url);
    ~Wallpaper();

    bool Initialize(LPCWSTR className, HINSTANCE hInstance);
    bool Resize();
    bool LoadURL(const wchar_t* url);
    void Disable(bool disable) const;

private:
    const wchar_t* m_url;
    HWND m_workerW;
    HWND m_window;
    HINSTANCE m_hInstance;
    BOOL m_initialized{false};

    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controller;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_webview;

    HWND FindWorkerW();
    bool CreateWallpaperWindow(LPCWSTR className);
    void InitWebView();
};
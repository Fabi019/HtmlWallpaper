# HtmlWallpaper

Small and lightweight (~14kb) win32 application that makes it possible to use any html website as the desktop wallper.
As the WebView2 sdk is used for rendering the website it needs to be installed seperatly for it to function (https://developer.microsoft.com/de-de/microsoft-edge/webview2).
Links to **msvcrt** to generate a minimal binary without requiring an additional c-runtime (CRT/STL) installed.

## Settings

All settings are loaded at the start of the program from a settings.ini file. This file has to be in the same folder as the executable. When the program can't find the file, it uses default hardcoded settings. To adjust them, click on the Settings entry in the tray menu. If no config file exists, it will ask you to create one. After pressing Yes a editor with the file should open up. The contents will look like the following:

```ini
[Settings]
Url=https://example.com				; Url to load
ClearCache=0						; Wether to clear the cache of the WebView on exit
AutoStart=0                         ; 1 = start with Windows, 0 = do not start with Windows
EfficiencyMode=1                    ; 1 = enable the EcoQos mode for lower CPU usage
```

Enabling `AutoStart` will create a entry in the Registry at `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`. This will enable the appilcation to launch when the computer starts. Disabling this setting will remove the entry on the next start of the application.
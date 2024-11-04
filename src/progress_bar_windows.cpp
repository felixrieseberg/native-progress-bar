#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <shellscalingapi.h>
#include <string>
#include "progress_bar_windows.h"

#define DEFAULT_WINDOW_WIDTH 1035
#define DEFAULT_WINDOW_HEIGHT 400

// Add DPI awareness helper
int GetWindowDpiHelper(HWND hwnd) {
    // Windows 10 1607 or later has GetDpiForWindow built in
    HMODULE user32 = GetModuleHandle(L"user32.dll");
    typedef UINT (WINAPI *GetDpiForWindowFunc)(HWND);
    GetDpiForWindowFunc getDpiForWindow = 
        (GetDpiForWindowFunc)GetProcAddress(user32, "GetDpiForWindow");
    
    if (getDpiForWindow) {
        return getDpiForWindow(hwnd);
    }
    
    // Fallback to GetDeviceCaps for older Windows versions
    HDC hdc = GetDC(hwnd);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwnd, hdc);
    return dpi;
}

// Scale value based on DPI
int ScaleForDpi(int value, int dpi) {
    return MulDiv(value, dpi, 96);
}

// Window class name
const wchar_t* WINDOW_CLASS_NAME = L"ProgressBarWindow";

// Register the window class
bool RegisterProgressBarWindowClass() {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    return RegisterClassExW(&wc) != 0;
}

void* ShowProgressBarWindows(
    const char* title,
    const char* message,
    const char* windowStyle,
    const char** buttonLabels,
    size_t buttonCount,
    void (*callback)(int)) {

    // Set DPI awareness
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    // Register window class
    static bool registered = RegisterProgressBarWindowClass();
    if (!registered) {
        return nullptr;
    }

    // Convert char* to wstring
    std::wstring wTitle(title, title + strlen(title));
    std::wstring wMessage(message, message + strlen(message));
    
    // Create the window using our custom window class
    HWND hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,  // Use our custom window class
        wTitle.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!hwnd) {
        return nullptr;
    }

    // Get DPI for the window
    int dpi = GetWindowDpiHelper(hwnd);

    // Create message text with DPI-aware font
    HWND hMessage = CreateWindowExW(
        WS_EX_TRANSPARENT,  // Add transparent style to extended window style
        L"STATIC",
        wMessage.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
        ScaleForDpi(30, dpi),
        ScaleForDpi(20, dpi),
        ScaleForDpi(440, dpi),
        ScaleForDpi(20, dpi),
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    // Create DPI-aware font
    int fontSize = ScaleForDpi(18, dpi);  // Increased size
    HFONT hFont = CreateFontW(
        fontSize,                    // Height
        0,                          // Width
        0,                          // Escapement
        0,                          // Orientation
        FW_NORMAL,                  // Weight
        FALSE,                      // Italic
        FALSE,                      // Underline
        0,                          // StrikeOut
        ANSI_CHARSET,               // CharSet
        OUT_DEFAULT_PRECIS,         // OutPrecision
        CLIP_DEFAULT_PRECIS,        // ClipPrecision
        CLEARTYPE_QUALITY,          // Quality
        DEFAULT_PITCH | FF_SWISS,   // PitchAndFamily
        L"Segoe UI"                 // Font Name
    );

    // Apply font to message
    SendMessage(hMessage, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Make background transparent
    SetWindowLongW(hMessage, GWL_EXSTYLE, 
        GetWindowLongW(hMessage, GWL_EXSTYLE) | WS_EX_TRANSPARENT);

    // Set text color and make background transparent
    HDC hdc = GetDC(hMessage);
    SetBkMode(hdc, TRANSPARENT);
    ReleaseDC(hMessage, hdc);

    // Make text background transparent
    LONG_PTR msgStyle = GetWindowLongPtr(hMessage, GWL_STYLE);
    msgStyle |= SS_NOTIFY;  // Add SS_NOTIFY style
    SetWindowLongPtr(hMessage, GWL_STYLE, msgStyle);

    // Set window background color to system default
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)GetSysColorBrush(COLOR_3DFACE));

    // Make text background transparent
    SetWindowLongPtr(hMessage, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(hMessage, GWLP_WNDPROC));

    // Create progress bar
    HWND hProgress = CreateWindowExW(
        0,
        PROGRESS_CLASSW,
        NULL,
        WS_CHILD | WS_VISIBLE,
        ScaleForDpi(30, dpi),
        ScaleForDpi(50, dpi),  // Adjusted position to be below message
        ScaleForDpi(440, dpi),
        ScaleForDpi(24, dpi),
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    // Create buttons if provided
    int buttonWidth = ScaleForDpi(100, dpi);
    int buttonHeight = ScaleForDpi(32, dpi);
    int buttonSpacing = ScaleForDpi(20, dpi);
    int buttonY = ScaleForDpi(100, dpi);

    for (size_t i = 0; i < buttonCount; i++) {
        std::wstring wButtonLabel(buttonLabels[i], buttonLabels[i] + strlen(buttonLabels[i]));
        HWND hButton = CreateWindowExW(
            0,
            L"BUTTON",
            wButtonLabel.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            ScaleForDpi(30, dpi) + (i * (buttonWidth + buttonSpacing)),
            buttonY,
            buttonWidth,
            buttonHeight,
            hwnd,
            (HMENU)(i + 1),
            GetModuleHandle(NULL),
            NULL
        );

        // Apply same font to buttons
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    }

    // Store callback and other data
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)callback);

    // Show the window
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

void UpdateProgressBarWindows(
    void* handle,
    int progress,
    const char* message,
    bool updateButtons,
    const char** buttonLabels,
    size_t buttonCount,
    void (*callback)(int)) {
    
    HWND hwnd = (HWND)handle;
    if (!hwnd) return;

    int dpi = GetWindowDpiHelper(hwnd);

    // Find the progress bar window
    HWND hProgress = FindWindowExW(hwnd, NULL, PROGRESS_CLASSW, NULL);
    if (hProgress) {
        SendMessage(hProgress, PBM_SETPOS, progress, 0);
    }

    if (message) {
        // Find the message static control (first STATIC child window)
        HWND hMessage = FindWindowExW(hwnd, NULL, L"STATIC", NULL);
        if (hMessage) {
            std::wstring wMessage(message, message + strlen(message));
            SetWindowTextW(hMessage, wMessage.c_str());
        }
    }

    if (updateButtons) {
        // Remove existing buttons
        EnumChildWindows(hwnd, [](HWND hChild, LPARAM) -> BOOL {
            wchar_t className[256];
            GetClassNameW(hChild, className, sizeof(className)/sizeof(wchar_t));
            if (wcscmp(className, L"BUTTON") == 0) {
                DestroyWindow(hChild);
            }
            return TRUE;
        }, 0);

        // Create new buttons with DPI scaling
        int buttonWidth = ScaleForDpi(100, dpi);
        int buttonHeight = ScaleForDpi(32, dpi);
        int buttonSpacing = ScaleForDpi(20, dpi);
        int buttonY = ScaleForDpi(100, dpi);

        for (size_t i = 0; i < buttonCount; i++) {
            std::wstring wButtonLabel(buttonLabels[i], buttonLabels[i] + strlen(buttonLabels[i]));
            HWND hButton = CreateWindowExW(
                0,
                L"BUTTON",
                wButtonLabel.c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                ScaleForDpi(30, dpi) + (i * (buttonWidth + buttonSpacing)),
                buttonY,
                buttonWidth,
                buttonHeight,
                hwnd,
                (HMENU)(i + 1),
                GetModuleHandle(NULL),
                NULL
            );

            // Set button font
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        // Store new callback
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)callback);
    }
}

void CloseProgressBarWindows(void* handle) {
    HWND hwnd = (HWND)handle;
    if (hwnd) {
        DestroyWindow(hwnd);
    }
} 

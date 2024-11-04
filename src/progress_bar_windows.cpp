#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <shellscalingapi.h>
#include <string>
#include "progress_bar_windows.h"

#define DEFAULT_WINDOW_WIDTH 1035
#define DEFAULT_WINDOW_HEIGHT 250
#define DEFAULT_WINDOW_HEIGHT_WITH_BUTTONS 360

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

// Window procedure to handle button clicks and prevent closing
LRESULT CALLBACK ProgressBarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CLOSE) {
        return 0;  // Ignore close request
    }
    else if (msg == WM_COMMAND) {
        // Handle button clicks
        int buttonId = LOWORD(wParam);
        if (buttonId >= 1) {  // Our buttons start from ID 1
            void (*callback)(int) = (void (*)(int))GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (callback) {
                callback(buttonId - 1);  // Convert back to 0-based index
            }
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Register the window class
bool RegisterProgressBarWindowClass() {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = ProgressBarWndProc;  // Use our window procedure
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    return RegisterClassExW(&wc) != 0;
}

void* ShowProgressBarWindows(
    const char* title,
    const char* message,
    const char* style,
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
    
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Calculate window dimensions
    int windowWidth = DEFAULT_WINDOW_WIDTH;
    int windowHeight = buttonCount == 0 ? DEFAULT_WINDOW_HEIGHT : DEFAULT_WINDOW_HEIGHT_WITH_BUTTONS;  // Taller if we have buttons
    
    // Calculate center position
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;

    // Create the window using our custom window class
    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        WINDOW_CLASS_NAME,
        wTitle.c_str(),
        WS_POPUP | WS_CAPTION | WS_VISIBLE,
        windowX, windowY,  // Centered position
        windowWidth, windowHeight,
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
    int buttonSpacing = ScaleForDpi(10, dpi);  // Reduced spacing between buttons
    int buttonY = ScaleForDpi(100, dpi);

    // Calculate total width needed for all buttons
    int totalButtonWidth = (buttonWidth * buttonCount) + (buttonSpacing * (buttonCount - 1));
    
    // Start position for the first button (from right side)
    int startX = ScaleForDpi(470, dpi) - totalButtonWidth;  // 470 is window width (500) minus margin (30)

    for (size_t i = 0; i < buttonCount; i++) {
        std::wstring wButtonLabel(buttonLabels[i], buttonLabels[i] + strlen(buttonLabels[i]));
        HWND hButton = CreateWindowExW(
            0,
            L"BUTTON",
            wButtonLabel.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            startX + (i * (buttonWidth + buttonSpacing)),  // Position from right to left
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
        // Find the message static control
        HWND hMessage = FindWindowExW(hwnd, NULL, L"STATIC", NULL);
        if (hMessage) {
            std::wstring wMessage(message, message + strlen(message));
            SetWindowTextW(hMessage, wMessage.c_str());
        }
    }

    if (updateButtons) {
        // Remove existing buttons
        HWND hButton = NULL;
        while ((hButton = FindWindowExW(hwnd, hButton, L"BUTTON", NULL)) != NULL) {
            DestroyWindow(hButton);
        }

        // Get screen dimensions for centering
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
        // Calculate new window size
        int windowWidth = DEFAULT_WINDOW_WIDTH;
        int windowHeight = buttonCount == 0 ? DEFAULT_WINDOW_HEIGHT : DEFAULT_WINDOW_HEIGHT_WITH_BUTTONS;
        
        // Calculate new center position
        int windowX = (screenWidth - windowWidth) / 2;
        int windowY = (screenHeight - windowHeight) / 2;

        // Resize and reposition window
        SetWindowPos(hwnd, NULL, 
            windowX, windowY, windowWidth, windowHeight,
            SWP_NOZORDER);

        if (buttonCount > 0) {
            // Create new buttons with DPI scaling
            int buttonWidth = ScaleForDpi(100, dpi);
            int buttonHeight = ScaleForDpi(32, dpi);
            int buttonSpacing = ScaleForDpi(10, dpi);
            int buttonY = ScaleForDpi(100, dpi);

            // Calculate total width needed for all buttons
            int totalButtonWidth = (buttonWidth * buttonCount) + (buttonSpacing * (buttonCount - 1));
            int startX = ScaleForDpi(470, dpi) - totalButtonWidth;

            for (size_t i = 0; i < buttonCount; i++) {
                std::wstring wButtonLabel(buttonLabels[i], buttonLabels[i] + strlen(buttonLabels[i]));
                HWND hNewButton = CreateWindowExW(
                    0,
                    L"BUTTON",
                    wButtonLabel.c_str(),
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    startX + (i * (buttonWidth + buttonSpacing)),
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
                SendMessage(hNewButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            }

            // Store new callback
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)callback);

            // Force window to redraw
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
        }
    }
}

void CloseProgressBarWindows(void* handle) {
    HWND hwnd = (HWND)handle;
    if (hwnd) {
        DestroyWindow(hwnd);
    }
} 

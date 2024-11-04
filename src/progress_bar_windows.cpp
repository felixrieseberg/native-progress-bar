#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include "progress_bar_windows.h"

#pragma comment(lib, "comctl32.lib")

struct ButtonInfo {
    std::wstring label;
    ButtonCallback callback;
    int index;
    HWND hwnd;
};

struct ProgressBarData {
    HWND hwnd;
    HWND progressBar;
    HWND messageLabel;
    std::vector<ButtonInfo> buttons;
    std::wstring title;
    std::wstring message;
    bool isValid;
    std::thread messageLoop;
};

const int WINDOW_WIDTH = 400;
const int WINDOW_HEIGHT = 150;
const int WINDOW_HEIGHT_WITH_BUTTONS = 190;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ProgressBarData* data = (ProgressBarData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
        case WM_COMMAND: {
            if (data) {
                int buttonId = LOWORD(wParam);
                for (const auto& button : data->buttons) {
                    if (button.hwnd == (HWND)lParam) {
                        button.callback(button.index);
                        break;
                    }
                }
            }
            break;
        }
        case WM_CLOSE:
            if (data) {
                data->isValid = false;
                DestroyWindow(hwnd);
            }
            break;
        case WM_DESTROY:
            if (data) {
                data->isValid = false;
            }
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

std::wstring Utf8ToWide(const char* str) {
    if (!str) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    std::vector<wchar_t> buf(size);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, buf.data(), size);
    return std::wstring(buf.data());
}

void* ShowProgressBarWindows(const char* title, const char* message, const char* style,
                           const char** buttonLabels, int buttonCount, ButtonCallback callback) {
    auto data = std::make_unique<ProgressBarData>();
    data->isValid = true;

    // Start message loop in a separate thread
    data->messageLoop = std::thread([data = data.get()]() {
        WNDCLASSEX wc = {0};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"ProgressBarWindow";
        RegisterClassEx(&wc);

        DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
        int height = buttonCount > 0 ? WINDOW_HEIGHT_WITH_BUTTONS : WINDOW_HEIGHT;

        // Center the window
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - WINDOW_WIDTH) / 2;
        int y = (screenHeight - height) / 2;

        data->hwnd = CreateWindowEx(
            0,
            L"ProgressBarWindow",
            Utf8ToWide(title).c_str(),
            windowStyle,
            x, y, WINDOW_WIDTH, height,
            nullptr,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        SetWindowLongPtr(data->hwnd, GWLP_USERDATA, (LONG_PTR)data);

        // Create progress bar
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icex);

        data->progressBar = CreateWindowEx(
            0,
            PROGRESS_CLASS,
            nullptr,
            WS_CHILD | WS_VISIBLE,
            20, 50, WINDOW_WIDTH - 40, 20,
            data->hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        SendMessage(data->progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

        // Create message label
        data->messageLabel = CreateWindowEx(
            0,
            L"STATIC",
            Utf8ToWide(message).c_str(),
            WS_CHILD | WS_VISIBLE,
            20, 20, WINDOW_WIDTH - 40, 20,
            data->hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        // Create buttons
        if (buttonCount > 0) {
            int buttonWidth = 100;
            int buttonHeight = 30;
            int buttonSpacing = 10;
            int startX = WINDOW_WIDTH - buttonWidth - 20;
            int buttonY = height - 50;

            for (int i = 0; i < buttonCount; i++) {
                HWND buttonHwnd = CreateWindowEx(
                    0,
                    L"BUTTON",
                    Utf8ToWide(buttonLabels[i]).c_str(),
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    startX, buttonY, buttonWidth, buttonHeight,
                    data->hwnd,
                    nullptr,
                    GetModuleHandle(nullptr),
                    nullptr
                );

                ButtonInfo buttonInfo;
                buttonInfo.label = Utf8ToWide(buttonLabels[i]);
                buttonInfo.callback = callback;
                buttonInfo.index = i;
                buttonInfo.hwnd = buttonHwnd;
                data->buttons.push_back(buttonInfo);

                startX -= (buttonWidth + buttonSpacing);
            }
        }

        ShowWindow(data->hwnd, SW_SHOW);
        UpdateWindow(data->hwnd);

        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (!data->isValid) break;
        }
    });

    return data.release();
}

void UpdateProgressBarWindows(void* handle, double progress, const char* message,
                            bool updateButtons, const char** buttonLabels, int buttonCount, ButtonCallback callback) {
    auto* data = static_cast<ProgressBarData*>(handle);
    if (!data || !data->isValid) return;

    auto updateFunc = [=]() {
        SendMessage(data->progressBar, PBM_SETPOS, (WPARAM)progress, 0);

        if (message) {
            SetWindowText(data->messageLabel, Utf8ToWide(message).c_str());
        }

        if (updateButtons) {
            // Remove existing buttons
            for (const auto& button : data->buttons) {
                DestroyWindow(button.hwnd);
            }
            data->buttons.clear();

            // Add new buttons
            if (buttonCount > 0) {
                int buttonWidth = 100;
                int buttonHeight = 30;
                int buttonSpacing = 10;
                int startX = WINDOW_WIDTH - buttonWidth - 20;
                int buttonY = WINDOW_HEIGHT_WITH_BUTTONS - 50;

                for (int i = 0; i < buttonCount; i++) {
                    HWND buttonHwnd = CreateWindowEx(
                        0,
                        L"BUTTON",
                        Utf8ToWide(buttonLabels[i]).c_str(),
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        startX, buttonY, buttonWidth, buttonHeight,
                        data->hwnd,
                        nullptr,
                        GetModuleHandle(nullptr),
                        nullptr
                    );

                    ButtonInfo buttonInfo;
                    buttonInfo.label = Utf8ToWide(buttonLabels[i]);
                    buttonInfo.callback = callback;
                    buttonInfo.index = i;
                    buttonInfo.hwnd = buttonHwnd;
                    data->buttons.push_back(buttonInfo);

                    startX -= (buttonWidth + buttonSpacing);
                }
            }
        }
    };

    if (IsWindow(data->hwnd)) {
        SendMessage(data->hwnd, WM_APP, 0, (LPARAM)updateFunc);
    }
}

void CloseProgressBarWindows(void* handle) {
    auto* data = static_cast<ProgressBarData*>(handle);
    if (!data || !data->isValid) return;

    data->isValid = false;
    PostMessage(data->hwnd, WM_CLOSE, 0, 0);
    
    if (data->messageLoop.joinable()) {
        data->messageLoop.join();
    }

    delete data;
} 

#include "./include/settingsLoader.h"
#include <vector>
#include "resource.h"  // <--- Add this at the top of your main.cpp
#include <math.h>
// Now this line will work:
HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));
// #include "./include/globals.h"

using namespace ScreenPen;
bool InitD2D(HWND hwnd) {
    HRESULT hr;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_factory);
    if (FAILED(hr)) return false;

    RECT rc;
    GetClientRect(hwnd, &rc);

    hr = g_factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
        ),
        D2D1::HwndRenderTargetProperties(
            hwnd,
            D2D1::SizeU(rc.right, rc.bottom)
        ),
        &g_renderTarget
    );
    if (FAILED(hr)) return false;

    hr = g_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(1.f, 0.f, 0.f, 1.f), // red brush
        &g_brush
    );
    if (FAILED(hr)) return false;

    return true;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_HOTKEY:{
        switch(wParam){
            case 1: {
                if (IsWindowVisible(g_settingsWnd))
                    ShowWindow(g_settingsWnd, SW_HIDE);
                else {
                    ShowWindow(g_settingsWnd, SW_SHOW);
                    SetForegroundWindow(g_settingsWnd);
                }
                break;
            }
            case 2: { // CTRL+Z (undo)
                if (g_fullLines.size()) {
                    g_fullLines.pop_back();
                    
                    InvalidateRect(g_overlayWnd, nullptr, FALSE);
                }
                break;
            }
            case 3:{ // CTRL+SHIFT+Z (clear)
                g_fullLines.clear();
                InvalidateRect(g_overlayWnd, nullptr, FALSE);
                break;
            }

        }
           
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        if (g_renderTarget) {
            g_renderTarget->BeginDraw();

            // Almost fully transparent to force Windows compositor
            g_renderTarget->Clear(D2D1::ColorF(0, 0, 0, 0.01f));

            // Draw all stored lines
                for(const auto&fullIne : g_fullLines){
                    for (const auto& ln : fullIne) {
                        g_brush->SetColor(ln.color);
                        g_renderTarget->DrawLine(ln.a, ln.b, g_brush, ln.thickness);
                    }
                }
            
            g_renderTarget->EndDraw();
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_TIMER: {
        bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

        POINT curr;
        GetCursorPos(&curr);

        if (ctrlDown) {
            if (!g_drawing) {
                g_drawing = true;
                g_lastCursor = curr;
            } else {
                // Convert screen coords to client coords
                POINT lastClient = g_lastCursor;
                POINT currClient = curr;
                ScreenToClient(hwnd, &lastClient);
                ScreenToClient(hwnd, &currClient);
                if (std::sqrt(((currClient.x - lastClient.x) * (currClient.x - lastClient.x)) - ((currClient.y - lastClient.y > 0) * (currClient.y - lastClient.y > 0))) > 0){
                    Line line = {
                            D2D1::Point2F((FLOAT)lastClient.x, (FLOAT)lastClient.y),
                            D2D1::Point2F((FLOAT)currClient.x, (FLOAT)currClient.y), 
                            g_settings.lineColor,
                            g_settings.opacity,
                            g_settings.lineThickness
                        };
                    if(g_fullLines.size()){
                            g_fullLines[g_fullLines.size() - 1].push_back(line);
                    }else{
                        std::vector<Line> lines;
                        lines.push_back(line);
                        g_fullLines.push_back(lines);
                    }
                    g_lastCursor = curr;
                // Only invalidate when a new line is added
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
        } else {
            g_drawing = false;

            if(g_lines.size()){
                // pack into the fullLines
                g_fullLines.push_back(g_lines);

                g_lines.clear();
            }
        }

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE); // hide instead of destroy
        return 0;
    case WM_CREATE:
    {
        // Thickness slider
        HWND hSlider = CreateWindowEx(
            0, TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
            20, 30, 250, 30,
            hwnd, (HMENU)ID_SLIDER_THICKNESS,
            nullptr, nullptr
        );

        SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(1, 20));
        SendMessage(hSlider, TBM_SETPOS, TRUE, (LPARAM)g_settings.lineThickness);

        // Color picker button
        CreateWindowEx(
            0, L"BUTTON", L"Pick Color",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 80, 120, 30,
            hwnd, (HMENU)ID_BTN_COLOR,
            nullptr, nullptr
        );
    }
    return 0;
    case WM_HSCROLL:
    {
        HWND hSlider = (HWND)lParam;
        if (GetDlgCtrlID(hSlider) == ID_SLIDER_THICKNESS) {
            int value = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
            g_settings.lineThickness = (float)value;

            // Redraw overlay
            InvalidateRect(g_overlayWnd, nullptr, FALSE);

            // Optional: saveSettings(g_settings);
        }
    }
    return 0;
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_BTN_COLOR) {
            CHOOSECOLOR cc{};
            static COLORREF customColors[16]{};

            cc.lStructSize = sizeof(cc);
            cc.lpCustColors = customColors;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;

            cc.rgbResult = RGB(
                (BYTE)(g_settings.lineColor.r * 255),
                (BYTE)(g_settings.lineColor.g * 255),
                (BYTE)(g_settings.lineColor.b * 255)
            );

            if (ChooseColor(&cc)) {
                g_settings.lineColor = D2D1::ColorF(
                    GetRValue(cc.rgbResult) / 255.f,
                    GetGValue(cc.rgbResult) / 255.f,
                    GetBValue(cc.rgbResult) / 255.f,
                    1.0f
                );

                InvalidateRect(g_overlayWnd, nullptr, FALSE);

                // Optional: saveSettings(g_settings);
            }
        }
    }
    return 0;

    case WM_DESTROY:
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


// ------------------- Entry point -------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    const wchar_t OVERLAY_CLASS[]  = L"OverlayWindow";
    const wchar_t SETTINGS_CLASS[] = L"SettingsWindow";

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = OVERLAY_CLASS;
    RegisterClass(&wc);

    WNDCLASS swc = {};
    swc.lpfnWndProc = SettingsWndProc;
    swc.hInstance = hInstance;
    swc.lpszClassName = SETTINGS_CLASS;
    RegisterClass(&swc);


    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    g_overlayWnd = CreateWindowEx(
        0,
        OVERLAY_CLASS,
        L"Overlay",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr,
        hInstance, nullptr
    );


    if (!g_overlayWnd) return 0;
    
    g_settingsWnd = CreateWindowEx(
        0,
        SETTINGS_CLASS,
        L"ScreenPen Settings",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,
        nullptr, nullptr,
        hInstance, nullptr
    );
    if (!g_settingsWnd) return 0;

    // Extend frame for true transparency
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(g_overlayWnd, &margins);

    if (!InitD2D(g_overlayWnd)) {
        MessageBox(g_overlayWnd, L"Direct2D init failed", L"Error", MB_ICONERROR);
        return 0;
    }

    if(RegisterHotKey(g_overlayWnd, 1, MOD_ALT ,VK_SPACE)){
        // MessageBox(g_overlayWnd, L"Test", L"Test...", MB_OK);
    }
    RegisterHotKey(g_overlayWnd, 2, MOD_CONTROL, 'Z');              // Undo
    RegisterHotKey(g_overlayWnd, 3, MOD_CONTROL | MOD_SHIFT, 'Z');  // Clear


    ShowWindow(g_overlayWnd, SW_SHOW);
    LONG exStyle = GetWindowLong(g_overlayWnd, GWL_EXSTYLE);
    SetWindowLong(g_overlayWnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_APPWINDOW);
    ShowWindow(g_settingsWnd, SW_HIDE);

    UpdateWindow(g_overlayWnd);
    UpdateWindow(g_settingsWnd); 

    // Enable click-through (optional)
    SetWindowLong(g_overlayWnd, GWL_EXSTYLE, GetWindowLong(g_overlayWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);

    // Timer for input polling (~60 FPS)
    SetTimer(g_overlayWnd, 1, 16, nullptr);
// Example for Win32 API
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));
    SendMessage(g_overlayWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(g_overlayWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    if (g_brush) g_brush->Release();
    if (g_renderTarget) g_renderTarget->Release();
    if (g_factory) g_factory->Release();
    UnregisterHotKey(nullptr, 1);

    return 0;
}

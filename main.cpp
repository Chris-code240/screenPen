#include "./include/globals.h"
#include "./include/settingsLoader.h"

using namespace ScreenPen;
//ID2D1Factory* g_factory 
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

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        if (g_renderTarget) {
            g_renderTarget->BeginDraw();

            // Almost fully transparent to force Windows compositor
            g_renderTarget->Clear(D2D1::ColorF(0, 0, 0, 0.01f));

            // Draw all stored lines
            for (const auto& ln : g_lines) {
                g_brush->SetColor(g_settings.lineColor);
                g_renderTarget->DrawLine(ln.a, ln.b, g_brush, g_settings.lineThickness);
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

                g_lines.push_back({
                    D2D1::Point2F((FLOAT)lastClient.x, (FLOAT)lastClient.y),
                    D2D1::Point2F((FLOAT)currClient.x, (FLOAT)currClient.y)
                });

                g_lastCursor = curr;
                // Only invalidate when a new line is added
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        } else {
            g_drawing = false;
        }

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ------------------- Entry point -------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    const wchar_t CLASS_NAME[] = L"OverlayWindow";
    loadSettings(g_settings);
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(
        0 , // Transparent click-through added later
        // WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_APPWINDOW, // Transparent click-through added later
        CLASS_NAME,
        L"Overlay",
        WS_POPUP,
        0, 0, width, height,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) return 0;

    // Extend frame for true transparency
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    if (!InitD2D(hwnd)) {
        MessageBox(hwnd, L"Direct2D init failed", L"Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_APPWINDOW);

    UpdateWindow(hwnd);

    // Enable click-through (optional)
    SetWindowLong(hwnd, GWL_EXSTYLE,
        GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);

    // Timer for input polling (~60 FPS)
    SetTimer(hwnd, 1, 16, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    if (g_brush) g_brush->Release();
    if (g_renderTarget) g_renderTarget->Release();
    if (g_factory) g_factory->Release();

    return 0;
}

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <tchar.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <vector>

struct Line {
    D2D1_POINT_2F a;
    D2D1_POINT_2F b;
};

std::vector<Line> g_lines;


ID2D1Factory*  g_factory = nullptr;
ID2D1HwndRenderTarget*  g_renderTarget = nullptr;
bool g_drawing = false;
POINT g_lastCursor = {};
ID2D1SolidColorBrush* g_brush = nullptr;


bool InitD2D(HWND hwnd) {
    HRESULT hr;

    hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        &g_factory
    );
    if (FAILED(hr)) return false;

    RECT rc;
    GetClientRect(hwnd, &rc);

    hr = g_factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(
                DXGI_FORMAT_UNKNOWN,
                D2D1_ALPHA_MODE_PREMULTIPLIED
            )
        ),
        D2D1::HwndRenderTargetProperties(
            hwnd,
            D2D1::SizeU(rc.right, rc.bottom)
        ),
        &g_renderTarget
    );

    return SUCCEEDED(hr);
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        if (g_renderTarget) {
            g_renderTarget->BeginDraw();
            g_renderTarget->Clear(D2D1::ColorF(0, 0, 0, 0)); // fully transparent
            g_renderTarget->EndDraw();
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
case WM_TIMER: {
    bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000);

    POINT curr;
    GetCursorPos(&curr);

    if (ctrlDown) {
        if (!g_drawing) {
            g_drawing = true;
            g_lastCursor = curr;
        } else {
            // Store line segment (screen coordinates)
            g_lines.push_back({
                D2D1::Point2F((FLOAT)g_lastCursor.x, (FLOAT)g_lastCursor.y),
                D2D1::Point2F((FLOAT)curr.x, (FLOAT)curr.y)
            });
            g_lastCursor = curr;
        }
    } else {
        g_drawing = false;
    }

    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
}

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
// Register the window class.
const wchar_t CLASS_NAME[]  = TEXT("Sample Window Class");

WNDCLASS wc = { };

wc.lpfnWndProc   = WndProc;
wc.hInstance     = hInstance;
wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);
int width  = GetSystemMetrics(SM_CXSCREEN);
int height = GetSystemMetrics(SM_CYSCREEN);
HWND hwnd = CreateWindowEx(
    WS_EX_LAYERED,
    // WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
    CLASS_NAME,
    L"",
    WS_POPUP,
    0, 0, width, height,
    NULL,
    NULL,
    hInstance,
    NULL
);
MARGINS margins = { -1 };
DwmExtendFrameIntoClientArea(hwnd, &margins);


if (!InitD2D(hwnd)) {
    MessageBox(hwnd, L"Direct2D init failed", L"Error", MB_ICONERROR);
    return 0;
}


ShowWindow(hwnd, SW_SHOW);
SetTimer(hwnd, 1, 10, nullptr); // 100 FPS input polling

UpdateWindow(hwnd);


    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

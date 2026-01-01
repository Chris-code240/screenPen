#ifndef GLOBALS_H_INCLUDED
#define GLOBALS_H_INCLUDED

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <d2d1.h>
#include <dwmapi.h>
#include "./json/json.hpp"
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwmapi.lib")

namespace ScreenPen{
// ------------------- Data structures -------------------
struct Line {
    D2D1_POINT_2F a;
    D2D1_POINT_2F b;
};

std::vector<Line> g_lines;

// ------------------- Direct2D globals -------------------
ID2D1Factory* g_factory;
ID2D1HwndRenderTarget* g_renderTarget = nullptr;
ID2D1SolidColorBrush* g_brush = nullptr;
struct Settings {
    D2D1_COLOR_F lineColor;
    float lineThickness;
    float opacity = 1.0;
};

Settings  g_settings;

// D2D1_COLOR_F color = D2D1::ColorF(255.f, 255.f, 255.f, g_settings.opacity);

bool g_drawing = false;
POINT g_lastCursor = {};

}
#endif

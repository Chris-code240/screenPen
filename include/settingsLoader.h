#ifndef SETTINGS_LOADER_H_INCLUDED
#define SETTINGS_LOADER_H_INCLUDED

#include "./globals.h"
#include <fstream>
namespace ScreenPen{
    using json = nlohmann::json;

void loadSettings(ScreenPen::Settings &settings){
    json j;
    std::ifstream in("src/config.json");
    if (in.is_open()) {
        in >> j;    
        in.close();
    }
    int r = j["lineColor"][0];
    int g = j["lineColor"][1];
    int b = j["lineColor"][2];
    settings.lineThickness = j["lineThickness"];
    settings.opacity = j["lineOpacity"];
    settings.lineColor = D2D1::ColorF(r, g, b, settings.opacity);
    return;
}

void updateSettings(ScreenPen::Settings &settings){
    json j;
    j["lineColor"] = {settings.lineColor.r, settings.lineColor.g, settings.lineColor.b};
    j["lineThickness"] = settings.lineThickness;
    j["lineOpacity"] = settings.opacity;
    std::ofstream out("config.json");
    out << j.dump(4);
    return;
}

}

#endif
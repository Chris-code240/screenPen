#ifndef SETTINGS_LOADER_H_INCLUDED
#define SETTINGS_LOADER_H_INCLUDED

#include "./globals.h"
#include <fstream>
namespace ScreenPen{
using json = nlohmann::json;

void loadSettings(Settings &settings);
void updateSettings(Settings &settings);

}

#endif
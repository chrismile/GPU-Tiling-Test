//============================================================================
// Name        : PixelSync.cpp
// Author      : Christoph Neuhauser
// Copyright   : GPLv3
//============================================================================

#include <iostream>
#include <Utils/File/FileUtils.hpp>
#include <Utils/AppSettings.hpp>
#include <Graphics/Window.hpp>

#include "MainApp.hpp"

using namespace std;
using namespace sgl;

int main(int argc, char *argv[]) {
    // Initialize the filesystem utilities
    FileUtils::get()->initialize("gpu-tiling-test", argc, argv);

    // Load the file containing the app settings
    string settingsFile = FileUtils::get()->getConfigDirectory() + "settings.txt";
    AppSettings::get()->loadSettings(settingsFile.c_str());
    AppSettings::get()->getSettings().addKeyValue("window-multisamples", 0);
    //AppSettings::get()->getSettings().addKeyValue("windowSettings.debugContext", true);
#ifdef DATA_PATH
    if (!sgl::FileUtils::get()->directoryExists("Data") && !sgl::FileUtils::get()->directoryExists("../Data")) {
        sgl::AppSettings::get()->setDataDirectory(DATA_PATH);
    }
#endif
    AppSettings::get()->setLoadGUI();

    sgl::AppSettings::get()->createWindow();
    AppSettings::get()->initializeSubsystems();

    AppLogic *app = new TilingTestApp();
    app->run();
    delete app;

    AppSettings::get()->release();

    return 0;
}

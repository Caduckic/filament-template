#pragma once

#include "platform/platform.h"
#include "renderer/filament_renderer.h"
#include "renderer/model_asset.h"

#include <chrono>

class App {
public:
    bool initialize();
    void run();
    void shutdown();
    bool processEvents();
    void update();

#if defined(MY_PLATFORM_WEB)
    void frame();
#endif

private:
    Platform m_platform;
    FilamentRenderer m_renderer;
    
    std::chrono::steady_clock::time_point m_lastFrame;

    float m_rotation;
    float m_scrollAmount;

    ModelAsset m_asset;
};
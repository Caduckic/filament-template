#pragma once

#include "platform/platform.h"
#include "renderer/filament_renderer.h"

class App {
public:
    bool initialize();
    void run();
    void shutdown();

#if defined(MY_PLATFORM_WEB)
    void frame();
#endif

private:
    Platform m_platform;
    FilamentRenderer m_renderer;
};
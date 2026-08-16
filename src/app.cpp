#include "app.h"

#if defined(MY_PLATFORM_WEB)
#include <emscripten.h>

static App* g_app = nullptr;

static void mainLoop() {
    if (g_app) {
        g_app->frame();
    }
}
#endif

bool App::initialize() {

    if (!m_platform.initialize()) {
        return false;
    }

    if (!m_renderer.initialize(
            m_platform.getNativeWindow())) {

        m_platform.shutdown();
        return false;
    }

    return true;
}

void App::run() {

#if defined(MY_PLATFORM_WEB)

    g_app = this;

    emscripten_set_main_loop(
        mainLoop,
        0,
        1
    );

#else

    while (m_platform.processEvents()) {
        m_renderer.renderFrame();
    }

#endif
}

#if defined(MY_PLATFORM_WEB)

void App::frame() {

    if (!m_platform.processEvents()) {
        emscripten_cancel_main_loop();
        return;
    }

    m_renderer.renderFrame();
}

#endif

void App::shutdown() {

#if defined(MY_PLATFORM_WEB)
    emscripten_cancel_main_loop();
    g_app = nullptr;
#endif

    m_renderer.shutdown();
    m_platform.shutdown();
}
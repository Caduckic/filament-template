#include "app.h"

#include <math/vec3.h>
#include <iostream>

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
    m_lastFrame = std::chrono::steady_clock::now();

    if (!m_platform.initialize()) {
        return false;
    }

    if (!m_renderer.initialize(
            m_platform.getNativeWindow())) {

        m_platform.shutdown();
        return false;
    }

    m_asset = m_renderer.loadModel("assets/models/Insert Cabinet.glb");
    m_asset.setPos({0.0f,-1.0f,0.0f});
    m_rotation = 0.0f;

    return true;
}

bool App::processEvents() {
    m_scrollAmount = 0.0f;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        switch (event.type) {

            case SDL_EVENT_QUIT:

                std::printf("SDL_QUIT received.\n");

                return false;

            case SDL_EVENT_KEY_DOWN:

                if (event.key.key == SDLK_ESCAPE) {

                    std::printf("Escape pressed.\n");

                    return false;
                }

                break;
            case SDL_EVENT_MOUSE_WHEEL: {
                // std::cout << "yep\n";
                m_scrollAmount += event.wheel.y;
                break;
            }

            default:
                break;
        }
    }

    return true;
}

void App::update() {
    // Time
    const auto now = std::chrono::steady_clock::now();

    const float deltaTime =
        std::chrono::duration<float>(now - m_lastFrame).count();
    // Input
    static float scrollVelocity = 0.0f;
    const float scrollFriction = 0.85f;  // Smooth exponential dampening
    const float scrollSensitivity = 15.0f;

    if (std::abs(m_scrollAmount) > 0.001f) {

        // Find Forward Vector (Up x Left)
        filament::math::float3 up = m_renderer.getCameraUp();
        filament::math::float3 left = m_renderer.getCameraLeft();

        filament::math::float3 forward;
        forward.x = (up.y * left.z) - (up.z * left.y);
        forward.y = (up.z * left.x) - (up.x * left.z);
        forward.z = (up.x * left.y) - (up.y * left.x);

        // Normalization
        float length = std::sqrt((forward.x * forward.x) + (forward.y * forward.y) + (forward.z * forward.z));
        if (length > 0.0001f) {
            forward.x /= length;
            forward.y /= length;
            forward.z /= length;
        }

        // Multiply by dt to guarantee smooth movement independent of system speed
        float moveStep = m_scrollAmount * 0.05f;

        // Apply to camera
        filament::math::float3 camPos = m_renderer.getCameraPos();
        camPos.x += forward.x * moveStep;
        camPos.y += forward.y * moveStep;
        camPos.z += forward.z * moveStep;
        // std::cout << moveStep << '\n';
        m_renderer.setCameraPos(camPos);

        // Apply smooth dampening friction
        m_scrollAmount *= scrollFriction; 
    }
    else {
        scrollVelocity = 0.0f; // Kill dead-zone drift
    }

    // Update
    m_lastFrame = now;

    // Rotate the triangle.
    m_rotation += 1.0f * deltaTime;

    if (m_rotation > 6.283185307f) {
        m_rotation -= 6.283185307f;
    }
    m_asset.setRot({0.0f, 1.0f, 0.0f}, m_rotation);
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

    while (processEvents()) {
        update();
        int x, y;
        SDL_GetWindowSizeInPixels(
            m_platform.getWindow(),
            &x,
            &y
        );
        m_renderer.renderFrame(x, y);
    }
#endif
}

#if defined(MY_PLATFORM_WEB)

void App::frame() {
    if (!processEvents()) {
        emscripten_cancel_main_loop();
        return;
    }

    update();
    int x, y;
    SDL_GetWindowSizeInPixels(
        m_platform.getWindow(),
        &x,
        &y
    );

    m_renderer.renderFrame(x, y);
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
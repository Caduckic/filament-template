#pragma once

#include <SDL.h>

struct SDL_Window;

class Platform {
public:
    Platform() = default;
    ~Platform();

    bool initialize();

    SDL_Window* getWindow() const;

    void* getNativeWindow() const;

    bool processEvents();

    void shutdown();

private:
    SDL_Window* m_window = nullptr;

#if defined(MY_PLATFORM_WEB)
    SDL_GLContext m_glContext = nullptr;
#endif

    void* m_nativeWindow = nullptr;

    bool m_running = false;
};
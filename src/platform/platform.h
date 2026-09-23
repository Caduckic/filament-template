#pragma once

#include <SDL3/SDL.h>

#if defined(MY_PLATFORM_LINUX)

#include <wayland-client.h>

struct FilamentWaylandWindow {
    wl_display* display;
    wl_surface* surface;
    uint32_t width;
    uint32_t height;
};

#endif

struct SDL_Window;

enum class NativeWindowType {
    None,
    X11,
    Wayland,
    Win32,
    Cocoa
};

class Platform {
public:
    Platform() = default;
    ~Platform();

    bool initialize();

    SDL_Window* getWindow() const;

    void* getNativeWindow() const;

    void shutdown();

private:
    SDL_Window* m_window = nullptr;

#if defined(MY_PLATFORM_WEB)
    SDL_GLContext m_glContext = nullptr;
#elif defined(MY_PLATFORM_LINUX)
    FilamentWaylandWindow m_waylandWindow{};
#endif
    NativeWindowType m_nativeWindowType;
    void* m_nativeWindow = nullptr;

    bool m_running = false;
};
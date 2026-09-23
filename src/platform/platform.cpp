#include "platform.h"

#include <cstdio>
#include <GLES3/gl3.h>

Platform::~Platform() {
    shutdown();
}

bool Platform::initialize() {

    if (!SDL_Init(SDL_INIT_VIDEO)) {

        std::fprintf(
            stderr,
            "SDL_Init failed: %s\n",
            SDL_GetError()
        );

        return false;
    }

#if defined(MY_PLATFORM_WEB)

    // WebGL 2 / OpenGL ES 3 context.
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_ES
    );

    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_MAJOR_VERSION,
        3
    );

    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_MINOR_VERSION,
        0
    );

#endif

    Uint32 windowFlags =
        SDL_WINDOW_RESIZABLE;

#if defined(MY_PLATFORM_WEB)

    windowFlags |= SDL_WINDOW_OPENGL;

#endif

    m_window = SDL_CreateWindow(
        "My Filament App",
        1280,
        720,
        windowFlags
    );

    if (!m_window) {

        std::fprintf(
            stderr,
            "SDL_CreateWindow failed: %s\n",
            SDL_GetError()
        );

        SDL_Quit();

        return false;
    }

#if defined(MY_PLATFORM_WEB)

    m_glContext = SDL_GL_CreateContext(m_window);

    if (!m_glContext) {

        std::fprintf(
            stderr,
            "SDL_GL_CreateContext failed: %s\n",
            SDL_GetError()
        );

        SDL_DestroyWindow(m_window);
        m_window = nullptr;

        SDL_Quit();

        return false;
    }

    std::printf(
        "WebGL context created successfully.\n"
    );

#else
    SDL_PropertiesID properties = SDL_GetWindowProperties(m_window);
    if (!properties) {
        std::fprintf(
            stderr,
            "SDL_GetWindowProperties failed: %s\n",
            SDL_GetError()
        );
        SDL_DestroyWindow(m_window);
        m_window = nullptr;

        SDL_Quit();
        
        return false;
    }

#if defined(MY_PLATFORM_LINUX)

    const char* driver = SDL_GetCurrentVideoDriver();

    if (driver && SDL_strcmp(driver, "x11") == 0) {
        auto x11Window = SDL_GetNumberProperty(
            properties,
            SDL_PROP_WINDOW_X11_WINDOW_NUMBER,
            0
        );
        if (x11Window == 0) {
            std::fprintf(
                stderr,
                "Failed to get X11 window handle: %s\n",
                SDL_GetError()
            );

            SDL_DestroyWindow(m_window);
            m_window = nullptr;

            SDL_Quit();

            return false;
        }

        m_nativeWindowType = NativeWindowType::X11;
        m_nativeWindow =
            reinterpret_cast<void*>(
                static_cast<uintptr_t>(x11Window)
            );
    }
    else if (driver && SDL_strcmp(driver, "wayland") == 0) {
        auto waylandDisplay = SDL_GetPointerProperty(
            properties,
            SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER,
            nullptr
        );

        auto waylandSurface = SDL_GetPointerProperty(
            properties,
            SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER,
            nullptr
        );

        if (!waylandDisplay || !waylandSurface) {
            std::fprintf(
                stderr,
                "Failed to get Wayland display/surface: %s\n",
                SDL_GetError()
            );

            SDL_DestroyWindow(m_window);
            m_window = nullptr;

            SDL_Quit();

            return false;
        }

        int width = 0;
        int height = 0;

        SDL_GetWindowSizeInPixels(
            m_window,
            &width,
            &height
        );

        m_waylandWindow.display =
            static_cast<wl_display*>(waylandDisplay);

        m_waylandWindow.surface =
            static_cast<wl_surface*>(waylandSurface);

        m_waylandWindow.width =
            static_cast<uint32_t>(width);

        m_waylandWindow.height =
            static_cast<uint32_t>(height);

        m_nativeWindowType =
            NativeWindowType::Wayland;

        m_nativeWindow =
            &m_waylandWindow;
    }

#endif

#endif

    m_running = true;

    return true;
}

SDL_Window* Platform::getWindow() const {
    return m_window;
}

void* Platform::getNativeWindow() const {
    return m_nativeWindow;
}

void Platform::shutdown() {

#if defined(MY_PLATFORM_WEB)

    if (m_glContext) {

        SDL_GL_DestroyContext(m_glContext);

        m_glContext = nullptr;
    }

#endif

    if (m_window) {

        SDL_DestroyWindow(m_window);

        m_window = nullptr;
        m_nativeWindow = nullptr;
    }

    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Quit();
    }

    m_running = false;
}
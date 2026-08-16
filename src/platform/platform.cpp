#include "platform.h"

#include <SDL.h>

#if defined(MY_PLATFORM_LINUX)
#include <SDL_syswm.h>
#endif

#include <cstdio>

Platform::~Platform() {
    shutdown();
}

bool Platform::initialize() {

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {

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
        SDL_WINDOW_SHOWN |
        SDL_WINDOW_RESIZABLE;

#if defined(MY_PLATFORM_WEB)

    windowFlags |= SDL_WINDOW_OPENGL;

#endif

    m_window = SDL_CreateWindow(
        "My Filament App",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
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

    if (SDL_GL_MakeCurrent(m_window, m_glContext) != 0) {

        std::fprintf(
            stderr,
            "SDL_GL_MakeCurrent failed: %s\n",
            SDL_GetError()
        );

        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;

        SDL_DestroyWindow(m_window);
        m_window = nullptr;

        SDL_Quit();

        return false;
    }

    std::printf(
        "WebGL context created successfully.\n"
    );

#else

    SDL_SysWMinfo windowInfo;

    SDL_VERSION(&windowInfo.version);

    if (!SDL_GetWindowWMInfo(m_window, &windowInfo)) {

        std::fprintf(
            stderr,
            "SDL_GetWindowWMInfo failed: %s\n",
            SDL_GetError()
        );

        SDL_DestroyWindow(m_window);
        m_window = nullptr;

        SDL_Quit();

        return false;
    }

#if defined(MY_PLATFORM_LINUX)

    m_nativeWindow =
        reinterpret_cast<void*>(
            windowInfo.info.x11.window
        );

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

bool Platform::processEvents() {

    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        switch (event.type) {

            case SDL_QUIT:

                std::printf("SDL_QUIT received.\n");

                m_running = false;
                break;

            case SDL_KEYDOWN:

                if (event.key.keysym.sym == SDLK_ESCAPE) {

                    std::printf("Escape pressed.\n");

                    m_running = false;
                }

                break;

            default:
                break;
        }
    }

    return m_running;
}

void Platform::shutdown() {

#if defined(MY_PLATFORM_WEB)

    if (m_glContext) {

        SDL_GL_DeleteContext(m_glContext);

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
#if defined(MY_PLATFORM_LINUX)
#include <SDL3/SDL_main.h>
#endif
#include "app.h"

int main() {

    App app;

    if (!app.initialize()) {
        return 1;
    }

    app.run();

    app.shutdown();

    return 0;
}
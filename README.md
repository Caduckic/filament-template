Requirements
- SDL2
- EMCC/EM++ (Web Only)
- Clang/Clang++ (C++ 20)
- libc++-dev
- libc++abi-dev

Build instructions for every platform

Prerequisites all Platforms(Build Tools)
CC=clang CXX=clang++ cmake -S external/filament -B build/filament-host -G Ninja -DCMAKE_BUILD_TYPE=Release -DFILAMENT_EXPORT_PREBUILT_EXECUTABLES_DIR=../../build filament-host
cmake --build build/filament-host -j$(nproc)

Examples:

Windows - TODO
cmake -S . -B build/windows -G Ninja
cmake --build build/windows

Linux
CC=clang CXX=clang++ cmake -S . -B build/linux -DFILAMENT_BUILD_TESTING=OFF
cmake --build build/linux -j$(nproc)

macOS - TODO
cmake -S . -B build/macos -G Ninja
cmake --build build/macos

Web:
emcmake cmake -S . -B build/web
cmake --build build/web -j$(nproc)
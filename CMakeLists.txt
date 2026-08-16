cmake_minimum_required(VERSION 3.22)

project(MyFilamentApp
    VERSION 0.1.0
    LANGUAGES CXX ASM
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------
# Platform detection
# ---------------------------------------------------------

if(EMSCRIPTEN)

    set(MY_PLATFORM "web")

    set(WASM ON CACHE BOOL
        "Build Filament for WebAssembly"
        FORCE
    )

    set(FILAMENT_SUPPORTS_OPENGL ON CACHE BOOL
        "Build Filament OpenGL backend"
        FORCE
    )

    set(FILAMENT_SUPPORTS_VULKAN OFF CACHE BOOL
        "Disable Vulkan for WebAssembly"
        FORCE
    )

    set(FILAMENT_SUPPORTS_WEBGPU OFF CACHE BOOL
        "Disable WebGPU"
        FORCE
    )

    set(FILAMENT_IMPORT_PREBUILT_EXECUTABLES_DIR
        "../../build/filament-host"
        CACHE PATH
        "Directory containing host Filament tools"
        FORCE
    )

    # set(CMAKE_EXE_LINKER_FLAGS
    #     "${CMAKE_EXE_LINKER_FLAGS} -sUSE_SDL=2 -sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=2 -sFULL_ES3=1"
    # )

    # Emscripten runtime settings
    add_link_options(
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_HEAP=268435456
        -sMAXIMUM_MEMORY=2147483648
        -sASSERTIONS=1
    )

elseif(WIN32)

    set(MY_PLATFORM "windows")

elseif(APPLE)

    set(MY_PLATFORM "macos")

elseif(UNIX)

    set(MY_PLATFORM "linux")

else()

    message(FATAL_ERROR "Unsupported platform")

endif()

message(STATUS "Building MyFilamentApp for: ${MY_PLATFORM}")

# ---------------------------------------------------------
# Filament
# ---------------------------------------------------------

set(FILAMENT_HOST_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/build/filament-host
)

if(EMSCRIPTEN)

    set(FILAMENT_PREBUILT_CMAKE
        ${FILAMENT_HOST_DIR}/ImportExecutables-Prebuilt.cmake
    )

    file(WRITE ${FILAMENT_PREBUILT_CMAKE}
"add_executable(matc IMPORTED)
set_property(TARGET matc PROPERTY IMPORTED_LOCATION \"${FILAMENT_HOST_DIR}/tools/matc/matc\")

add_executable(resgen IMPORTED)
set_property(TARGET resgen PROPERTY IMPORTED_LOCATION \"${FILAMENT_HOST_DIR}/tools/resgen/resgen\")

add_executable(cmgen IMPORTED)
set_property(TARGET cmgen PROPERTY IMPORTED_LOCATION \"${FILAMENT_HOST_DIR}/tools/cmgen/cmgen\")

add_executable(uberz IMPORTED)
set_property(TARGET uberz PROPERTY IMPORTED_LOCATION \"${FILAMENT_HOST_DIR}/tools/uberz/uberz\")

add_executable(glslminifier IMPORTED)
set_property(TARGET glslminifier PROPERTY IMPORTED_LOCATION \"${FILAMENT_HOST_DIR}/tools/glslminifier/glslminifier\")
")

endif()

set(FILAMENT_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/external/filament
)

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    # Use the system libstdc++ on Linux.
    set(USE_STATIC_LIBCXX OFF CACHE BOOL
        "Link against the static runtime libraries."
        FORCE
    )

    # Do NOT force libc++.
    string(REPLACE "-stdlib=libc++" "" CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS}"
    )
endif()

set(FILAMENT_SKIP_SAMPLES ON CACHE BOOL
    "Don't build Filament samples"
    FORCE
)

if(EMSCRIPTEN)
    add_compile_options(
        -Wno-unused-template
        -Wno-error=unused-template
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    string(APPEND CMAKE_CXX_FLAGS
        " -Wno-thread-safety"
        " -Wno-thread-safety-analysis"
        " -Wno-thread-safety-attributes"
        " -Wno-error=thread-safety"
        " -Wno-error=thread-safety-analysis"
        " -Wno-error=thread-safety-attributes"
    )
endif()

if(EMSCRIPTEN)
    string(APPEND CMAKE_CXX_FLAGS
        " -Wno-unused-template"
    )
endif()

add_subdirectory(
    ${FILAMENT_DIR}
    ${CMAKE_BINARY_DIR}/filament
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR EMSCRIPTEN)
    get_property(_filament_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)

    foreach(_target IN LISTS _filament_targets)
        get_target_property(_type ${_target} TYPE)

        if(_type STREQUAL "EXECUTABLE" OR
           _type STREQUAL "STATIC_LIBRARY" OR
           _type STREQUAL "SHARED_LIBRARY" OR
           _type STREQUAL "MODULE_LIBRARY")

            target_compile_options(${_target} PRIVATE
                -Wno-error=unused-template
            )

        endif()
    endforeach()
endif()

# Filament's thread-safety annotations currently produce diagnostics
# with Clang + Linux std::mutex. Disable those diagnostics for all
# targets created by Filament.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    get_property(_all_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)

    foreach(_target IN LISTS _all_targets)
        get_target_property(_type ${_target} TYPE)

        if(_type STREQUAL "EXECUTABLE" OR
           _type STREQUAL "STATIC_LIBRARY" OR
           _type STREQUAL "SHARED_LIBRARY" OR
           _type STREQUAL "MODULE_LIBRARY")
            target_compile_options(${_target} PRIVATE
                -Wno-thread-safety
                -Wno-thread-safety-analysis
                -Wno-thread-safety-attributes
                -Wno-error=thread-safety
                -Wno-error=thread-safety-analysis
                -Wno-error=thread-safety-attributes
            )
        endif()
    endforeach()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Filament adds -Werror and -Wthread-safety to its own targets.
    # Remove those from the global flags that Filament inherited.
    string(REPLACE "-Werror" "" CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS}"
    )
    string(REPLACE "-Wthread-safety" "" CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS}"
    )
endif()

# ---------------------------------------------------------
# Filament materials
# ---------------------------------------------------------

set(MATERIAL_OUTPUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/assets/materials
)

file(MAKE_DIRECTORY
    ${MATERIAL_OUTPUT_DIR}
)

set(TRIANGLE_MATERIAL
    ${MATERIAL_OUTPUT_DIR}/triangle.filamat
)

if(EMSCRIPTEN)
    set(MATERIAL_API opengl)
    set(MATERIAL_PROFILE mobile)
else()
    set(MATERIAL_API vulkan)
    set(MATERIAL_PROFILE desktop)
endif()

set(FILAMENT_MATC
    ${FILAMENT_HOST_DIR}/tools/matc/matc
)

add_custom_command(
    OUTPUT ${TRIANGLE_MATERIAL}

    COMMAND ${FILAMENT_MATC}
        -a ${MATERIAL_API}
        -p ${MATERIAL_PROFILE}
        -o ${TRIANGLE_MATERIAL}
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/materials/triangle.mat

    DEPENDS
        ${FILAMENT_MATC}
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/materials/triangle.mat

    COMMENT "Compiling triangle.mat for ${MATERIAL_API}/${MATERIAL_PROFILE}"
)

add_custom_target(MyFilamentMaterials
    DEPENDS
        ${TRIANGLE_MATERIAL}
)

# ---------------------------------------------------------
# Models
# ---------------------------------------------------------

set(MODEL_SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models
)

set(MODEL_OUTPUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/assets/models
)

add_custom_target(MyFilamentModels ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MODEL_SOURCE_DIR}
        ${MODEL_OUTPUT_DIR}

    COMMENT "Copying models"
)

if(EMSCRIPTEN)
    # Emscripten provides SDL2 through its system libraries.
else()
    find_package(SDL2 CONFIG REQUIRED)
endif()

# ---------------------------------------------------------
# Application
# ---------------------------------------------------------

add_executable(MyFilamentApp

    src/main.cpp

    src/app.cpp
    src/platform/platform.cpp
    src/renderer/filament_renderer.cpp
    src/renderer/model_loader.cpp

)

if(EMSCRIPTEN)

    target_compile_options(MyFilamentApp PRIVATE
        -sUSE_SDL=2
    )

    target_link_options(MyFilamentApp PRIVATE
        -sUSE_SDL=2
        -sMAX_WEBGL_VERSION=2
        -sMIN_WEBGL_VERSION=2
        -sFULL_ES3=1

        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets/models@/assets/models"
        "SHELL:--preload-file ${MATERIAL_OUTPUT_DIR}/triangle.filamat@/assets/materials/triangle.filamat"
    )

endif()

add_dependencies(
    MyFilamentApp
    MyFilamentMaterials
    MyFilamentModels
)

target_include_directories(MyFilamentApp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------------------------
# Filament
# ---------------------------------------------------------

target_link_libraries(MyFilamentApp
    PRIVATE
        filament
        utils
)

if(TARGET gltfio)
    target_link_libraries(MyFilamentApp PRIVATE gltfio)
elseif(TARGET gltfio_core)
    target_link_libraries(MyFilamentApp PRIVATE gltfio_core uberarchive)
else()
    message(FATAL_ERROR "Filament gltfio target not found")
endif()

if(EMSCRIPTEN)
    # target_link_options(MyFilamentApp PRIVATE
    #     -sUSE_SDL=2
    # )
else()
    target_link_libraries(MyFilamentApp PRIVATE
        SDL2::SDL2
    )
endif()

# ---------------------------------------------------------
# SDL2
# ---------------------------------------------------------

# target_link_libraries(MyFilamentApp
#     PRIVATE
#         SDL2::SDL2
# )

# ---------------------------------------------------------
# Platform definitions
# ---------------------------------------------------------

if(EMSCRIPTEN)

    target_compile_definitions(MyFilamentApp
        PRIVATE
            MY_PLATFORM_WEB
    )

elseif(WIN32)

    target_compile_definitions(MyFilamentApp
        PRIVATE
            MY_PLATFORM_WINDOWS
    )

elseif(APPLE)

    target_compile_definitions(MyFilamentApp
        PRIVATE
            MY_PLATFORM_MACOS
    )

elseif(UNIX)

    target_compile_definitions(MyFilamentApp
        PRIVATE
            MY_PLATFORM_LINUX
    )

endif()

# ---------------------------------------------------------
# Warnings
# ---------------------------------------------------------

if(MSVC)

    target_compile_options(MyFilamentApp PRIVATE
        /W4
    )

else()

    target_compile_options(MyFilamentApp PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

endif()
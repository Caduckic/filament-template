#include "model_loader.h"

#include <fstream>
#include <cstdio>

#include <gltfio/TextureProvider.h>

#ifdef MY_PLATFORM_WEB
#include <materials/uberarchive.h>
#endif
// #include <gltfio/StbProvider.h>

using namespace filament;
using namespace filament::gltfio;

ModelLoader::~ModelLoader() {
    shutdown();
}

std::vector<uint8_t> ModelLoader::readFile(
        const char* path) {

    std::ifstream file(
        path,
        std::ios::binary | std::ios::ate
    );

    if (!file) {
        std::fprintf(
            stderr,
            "Failed to open model: %s\n",
            path
        );

        return {};
    }

    const std::streamsize size = file.tellg();

    if (size <= 0) {
        return {};
    }

    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(
        static_cast<size_t>(size)
    );

    if (!file.read(
        reinterpret_cast<char*>(data.data()),
        size
    )) {
        std::fprintf(
            stderr,
            "Failed to read model: %s\n",
            path
        );

        return {};
    }

    return data;
}

bool ModelLoader::initialize(filament::Engine* engine) {

    m_engine = engine;

    if (!m_engine) {
        std::fprintf(stderr, "MODEL INIT: no engine\n");
        return false;
    }

    // -----------------------------------------------------
    // Material provider
    // -----------------------------------------------------

#ifdef MY_PLATFORM_WEB

    m_materialProvider =
        createUbershaderProvider(
            m_engine,
            UBERARCHIVE_DEFAULT_DATA,
            UBERARCHIVE_DEFAULT_SIZE
        );

#else

    m_materialProvider =
        createJitShaderProvider(m_engine);

#endif

    if (!m_materialProvider) {
        std::fprintf(
            stderr,
            "Failed to create glTF material provider.\n"
        );

        return false;
    }

    // -----------------------------------------------------
    // Asset loader
    // -----------------------------------------------------

    m_assetLoader =
        AssetLoader::create({
            m_engine,
            m_materialProvider
        });

    if (!m_assetLoader) {
        std::fprintf(
            stderr,
            "Failed to create AssetLoader.\n"
        );

        return false;
    }

    // -----------------------------------------------------
    // Resource loader
    // -----------------------------------------------------

#ifdef MY_PLATFORM_WEB

    m_resourceLoader =
        new ResourceLoader({
            m_engine,
            nullptr,
            false
        });

#else

    m_resourceLoader =
        new ResourceLoader({
            m_engine,
            nullptr,
            true
        });

#endif

    if (!m_resourceLoader) {
        std::fprintf(
            stderr,
            "Failed to create ResourceLoader.\n"
        );

        return false;
    }

    // -----------------------------------------------------
    // JPEG / PNG provider
    // -----------------------------------------------------

    m_textureProvider =
        createStbProvider(m_engine);

    if (!m_textureProvider) {
        std::fprintf(
            stderr,
            "Failed to create STB texture provider.\n"
        );

        return false;
    }

    m_resourceLoader->addTextureProvider(
        "image/jpeg",
        m_textureProvider
    );

    m_resourceLoader->addTextureProvider(
        "image/png",
        m_textureProvider
    );

    return true;
}

FilamentAsset* ModelLoader::load(
        const char* path,
        filament::Scene* scene) {

    std::printf("MODEL LOAD: entered\n");

    if (!m_engine) {
        std::printf("MODEL LOAD: no engine\n");
        return nullptr;
    }

    if (!m_assetLoader) {
        std::printf("MODEL LOAD: no asset loader\n");
        return nullptr;
    }

    if (!m_resourceLoader) {
        std::printf("MODEL LOAD: no resource loader\n");
        return nullptr;
    }

    if (!scene) {
        std::printf("MODEL LOAD: no scene\n");
        return nullptr;
    }

    std::printf("MODEL LOAD: reading %s\n", path);

    const auto data = readFile(path);

    std::printf(
        "MODEL LOAD: read %zu bytes\n",
        data.size()
    );

    if (data.empty()) {
        std::fprintf(
            stderr,
            "MODEL LOAD: file is empty\n"
        );
        return nullptr;
    }

    std::printf("MODEL LOAD: createAsset\n");

    FilamentAsset* asset =
        m_assetLoader->createAsset(
            data.data(),
            static_cast<uint32_t>(data.size())
        );

    std::printf(
        "MODEL LOAD: createAsset returned %p\n",
        (void*)asset
    );

    if (!asset) {
        std::fprintf(
            stderr,
            "MODEL LOAD: createAsset FAILED\n"
        );
        return nullptr;
    }

#ifdef MY_PLATFORM_WEB

    std::printf("MODEL LOAD: asyncBeginLoad\n");

    const bool resourcesLoaded =
        m_resourceLoader->asyncBeginLoad(asset);

    std::printf(
        "MODEL LOAD: asyncBeginLoad returned %s\n",
        resourcesLoaded ? "true" : "false"
    );

    if (!resourcesLoaded) {
        std::fprintf(
            stderr,
            "MODEL LOAD: asyncBeginLoad FAILED\n"
        );

        m_assetLoader->destroyAsset(asset);

        return nullptr;
    }

    // Web build has threading disabled, so we must manually
    // advance the asynchronous resource loader.
    while (m_resourceLoader->asyncGetLoadProgress() < 1.0f) {
        m_resourceLoader->asyncUpdateLoad();
    }

    std::printf(
        "MODEL LOAD: async loading complete (progress=%f)\n",
        m_resourceLoader->asyncGetLoadProgress()
    );

#else

    std::printf("MODEL LOAD: loadResources\n");

    const bool resourcesLoaded =
        m_resourceLoader->loadResources(asset);

    std::printf(
        "MODEL LOAD: loadResources returned %s\n",
        resourcesLoaded ? "true" : "false"
    );

    if (!resourcesLoaded) {
        std::fprintf(
            stderr,
            "MODEL LOAD: loadResources FAILED\n"
        );

        m_assetLoader->destroyAsset(asset);

        return nullptr;
    }

#endif

    std::printf("MODEL LOAD: adding entities\n");

    scene->addEntities(
        asset->getEntities(),
        asset->getEntityCount()
    );

    std::printf("MODEL LOAD: entities added\n");

    asset->releaseSourceData();

    std::printf(
        "MODEL LOAD: SUCCESS (%zu entities)\n",
        asset->getEntityCount()
    );

    return asset;
}

void ModelLoader::unload(
        FilamentAsset* asset,
        filament::Scene* scene) {

    if (!asset || !scene || !m_assetLoader) {
        return;
    }

    scene->removeEntities(
        asset->getEntities(),
        asset->getEntityCount()
    );

    m_assetLoader->destroyAsset(asset);
}

void ModelLoader::shutdown() {

    if (m_resourceLoader) {
        delete m_resourceLoader;
        m_resourceLoader = nullptr;
    }

    if (m_textureProvider) {
        delete m_textureProvider;
        m_textureProvider = nullptr;
    }

    if (m_assetLoader) {
        AssetLoader::destroy(&m_assetLoader);
    }

    if (m_materialProvider) {
        m_materialProvider->destroyMaterials();
        delete m_materialProvider;
        m_materialProvider = nullptr;
    }

    m_engine = nullptr;
}
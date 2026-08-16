#pragma once

#include <filament/Engine.h>
#include <filament/Scene.h>

#include <gltfio/AssetLoader.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/MaterialProvider.h>
#include <gltfio/ResourceLoader.h>

#include <cstdint>
#include <vector>

class ModelLoader {

public:

    ModelLoader() = default;
    ~ModelLoader();

    bool initialize(filament::Engine* engine);

    filament::gltfio::FilamentAsset* load(
        const char* path,
        filament::Scene* scene
    );

    void unload(
        filament::gltfio::FilamentAsset* asset,
        filament::Scene* scene
    );

    void shutdown();

private:

    static std::vector<uint8_t> readFile(
        const char* path
    );

    filament::Engine* m_engine = nullptr;

    filament::gltfio::MaterialProvider* m_materialProvider = nullptr;

    filament::gltfio::AssetLoader* m_assetLoader = nullptr;

    filament::gltfio::ResourceLoader* m_resourceLoader = nullptr;

    filament::gltfio::TextureProvider* m_textureProvider = nullptr;
};
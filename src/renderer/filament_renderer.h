#pragma once

#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/SwapChain.h>
#include <filament/Camera.h>
#include <filament/Viewport.h>
#include <filament/VertexBuffer.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <filament/LightManager.h>
#include <filament/IndirectLight.h>

#include <utils/Entity.h>

#include <vector>

#include "model_loader.h"
#include "model_asset.h"

class FilamentRenderer {
public:
    FilamentRenderer();
    ~FilamentRenderer();

    bool initialize(void* nativeWindow);

    void createLighting();

    void renderFrame(const uint32_t x, const uint32_t y);

    void shutdown();

    filament::math::float3 getCameraPos();
    filament::math::float3 getCameraTarget();
    filament::math::float3 getCameraUp();
    filament::math::float3 getCameraLeft();

    void setCameraPos(filament::math::float3 pos);
    void setCameraTarget(filament::math::float3 tar);
    void setCameraUp(filament::math::float3 up);

    ModelAsset loadModel(const char* path);

private:
    filament::Engine* m_engine = nullptr;
    filament::Renderer* m_renderer = nullptr;
    filament::Scene* m_scene = nullptr;
    filament::View* m_view = nullptr;
    filament::SwapChain* m_swapChain = nullptr;

    filament::Camera* m_camera = nullptr;
    utils::Entity m_cameraEntity;
    filament::math::float3 m_cameraTarget;

    bool m_cameraCreated = false;

    ModelLoader m_modelLoader;

    std::vector<filament::gltfio::FilamentAsset*> m_models;

    utils::Entity m_dirLightEntity;
    filament::IndirectLight* m_ambLight = nullptr;

    filament::math::int2 m_lastScreenSize;
};
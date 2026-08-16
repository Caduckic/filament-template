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

#include <chrono>

#include "model_loader.h"

class FilamentRenderer {
public:
    FilamentRenderer();
    ~FilamentRenderer();

    bool initialize(void* nativeWindow);

    void createLighting();

    void renderFrame();

    void shutdown();

private:
    filament::Engine* m_engine = nullptr;
    filament::Renderer* m_renderer = nullptr;
    filament::Scene* m_scene = nullptr;
    filament::View* m_view = nullptr;
    filament::SwapChain* m_swapChain = nullptr;

    filament::Camera* m_camera = nullptr;
    utils::Entity m_cameraEntity;

    bool m_cameraCreated = false;

    struct Vertex {
        filament::math::float3 position;
        filament::math::float4 color;
    };

    filament::VertexBuffer* m_vertexBuffer = nullptr;
    filament::IndexBuffer* m_indexBuffer = nullptr;

    filament::Material* m_material = nullptr;
    filament::MaterialInstance* m_materialInstance = nullptr;

    utils::Entity m_triangleEntity{};

    float m_rotation = 0.0f;

    std::chrono::steady_clock::time_point m_lastFrame;

    ModelLoader m_modelLoader;

    filament::gltfio::FilamentAsset* m_model = nullptr;

    utils::Entity m_lightEntity;
    utils::Entity m_fillLight;
    filament::IndirectLight* m_indirectLight = nullptr;
};
#pragma once

#include <gltfio/FilamentAsset.h>

class ModelAsset {
private:
    filament::gltfio::FilamentAsset* m_model;
    filament::Engine* m_engine;
    filament::Scene* m_scene;

    filament::math::float3 m_position;
    filament::math::quatf m_rotation;
public:
    ModelAsset();
    ModelAsset(filament::gltfio::FilamentAsset* model, filament::Engine* engine, filament::Scene* scene);
    ~ModelAsset();

    void setPos(const filament::math::float3& pos);
    void setRot(const filament::math::float3& axis, const float& angle);
};
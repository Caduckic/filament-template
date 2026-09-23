#include "model_asset.h"

#include <filament/Engine.h>
#include <filament/TransformManager.h>

void QuatToAxisAngle(const filament::math::quatf& quat, filament::math::float3& outAxis, float& outAngle) {
    float qw = std::clamp(quat.w, -1.0f, 1.0f);
    
    outAngle = 2.0f * std::acos(qw);
    
    float s = std::sqrt(1.0f - qw * qw);
    
    // Check if s is too close to zero to avoid division by zero (identity rotation)
    if (s < 0.00001f) {
        outAxis = filament::math::float3(1.0f, 0.0f, 0.0f); // Default axis
    } else {
        outAxis = filament::math::float3(quat.x, quat.y, quat.z) / s;
    }
}

ModelAsset::ModelAsset()
    : m_model(nullptr), m_engine(nullptr), m_scene(nullptr), m_position{0.0f,0.0f,0.0f}, m_rotation{} {};
ModelAsset::ModelAsset(filament::gltfio::FilamentAsset* model, filament::Engine* engine, filament::Scene* scene)
    : m_model{model}, m_engine{engine}, m_scene{scene}, m_position{0.0f,0.0f,0.0f}, m_rotation{} {};
ModelAsset::~ModelAsset() = default;

void ModelAsset::setPos(const filament::math::float3& pos) {
    if (!m_engine) {
        return;
    }
    auto& transformManager =
        m_engine->getTransformManager();
    auto instance =
        transformManager.getInstance(m_model->getRoot());

    if (!instance) {
        return;
    }

    m_position = pos;

    filament::math::float3 axis;
    float angle;
    QuatToAxisAngle(m_rotation, axis, angle);

    transformManager.setTransform(
        instance,
        filament::math::mat4f::translation(
            pos
        )
        *
        filament::math::mat4f::rotation(
            angle,
            axis
        )
    );
}

void ModelAsset::setRot(const filament::math::float3& axis, const float& angle) {
    if (!m_engine) {
        return;
    }
    auto& transformManager =
        m_engine->getTransformManager();
    auto instance =
        transformManager.getInstance(m_model->getRoot());

    if (!instance) {
        return;
    }

    m_rotation.fromAxisAngle(axis, angle);

    transformManager.setTransform(
        instance,
        filament::math::mat4::translation(
            m_position
        )
        *
        filament::math::mat4f::rotation(
            angle,
            axis
        )
    );
}
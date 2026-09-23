#include "filament_renderer.h"

#include <utils/EntityManager.h>

#include <fstream>
#include <vector>

ModelAsset FilamentRenderer::loadModel(const char* path) {
    filament::gltfio::FilamentAsset* m_model = m_modelLoader.load(
        "assets/models/Insert Cabinet.glb",
        m_scene
    );

    std::printf("INIT: model = %p\n", (void*)m_model);

    if (!m_model) {
        shutdown();
    }

    m_models.emplace_back(m_model);
    return {m_model, m_engine, m_scene};
}

static std::vector<uint8_t> readFile(const char* path) {

    std::ifstream file(
        path,
        std::ios::binary | std::ios::ate
    );

    if (!file) {
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
        return {};
    }

    return data;
}

void FilamentRenderer::createLighting() {

    auto& em = m_engine->getEntityManager();

    m_dirLightEntity = em.create();

    filament::LightManager::Builder(
        filament::LightManager::Type::DIRECTIONAL
    )
        .color({1.0f, 1.0f, 1.0f})
        .intensity(100000.0f)
        .direction({-0.5f, -1.0f, -0.5f})
        .castShadows(true)
        .build(
            *m_engine,
            m_dirLightEntity
        );

    m_scene->addEntity(m_dirLightEntity);

    constexpr float ambient = 0.25f;

    // 9 spherical-harmonic coefficients, each with RGB.
    // We want a constant environment, so only the first coefficient matters.
    filament::math::float3 sh[9] = {
        filament::math::float3{ambient, ambient, ambient},
        filament::math::float3{0.0f},
        filament::math::float3{0.0f},
        filament::math::float3{0.0f},
        filament::math::float3{0.0f},
        filament::math::float3{0.0f},
        filament::math::float3{0.0f},
        filament::math::float3{0.0f},
        filament::math::float3{0.0f}
    };

    m_ambLight =
        filament::IndirectLight::Builder()
            .irradiance(3, sh)
            .intensity(30000.0f)
            .build(*m_engine);

    if (!m_ambLight) {
        shutdown();
        // return false;
    }

    m_scene->setIndirectLight(m_ambLight);
}

FilamentRenderer::FilamentRenderer()
    : m_engine(nullptr),
      m_swapChain(nullptr),
      m_renderer(nullptr),
      m_scene(nullptr),
      m_view(nullptr),
      m_camera(nullptr),
      m_cameraEntity{},
      m_cameraCreated(false),
      m_models{},
      m_lastScreenSize(filament::math::int2{1280, 720}) {
}

FilamentRenderer::~FilamentRenderer() {
    shutdown();
}

bool FilamentRenderer::initialize(void* nativeWindow) {

    // -----------------------------------------------------
    // Engine
    // -----------------------------------------------------

    m_engine = filament::Engine::create();

    if (!m_engine) {
        return false;
    }

    // -----------------------------------------------------
    // Core rendering objects
    // -----------------------------------------------------

#if defined(MY_PLATFORM_WEB)
    m_swapChain = m_engine->createSwapChain(nullptr);

#else

    m_swapChain = m_engine->createSwapChain(nativeWindow);

#endif

    m_renderer = m_engine->createRenderer();

    m_scene = m_engine->createScene();

    m_view = m_engine->createView();

    if (!m_renderer ||
        !m_scene ||
        !m_view ||
        !m_swapChain) {

        shutdown();
        return false;
    }

    createLighting();

    if (!m_modelLoader.initialize(m_engine)) {
        std::printf("INIT: model loader FAILED\n");
        shutdown();
        return false;
    }

    // -----------------------------------------------------
    // Camera entity
    // -----------------------------------------------------

    utils::EntityManager& entityManager =
        m_engine->getEntityManager();

    m_cameraEntity = entityManager.create();

    m_camera = m_engine->createCamera(m_cameraEntity);

    if (!m_camera) {
        entityManager.destroy(m_cameraEntity);
        m_cameraEntity = {};
        shutdown();
        return false;
    }

    m_cameraCreated = true;

    const double near = 0.1;
    const double far = 100.0;

    const double aspect = (double)m_lastScreenSize.x / (double)m_lastScreenSize.y;
    const double fovY = filament::math::F_PI / 4.0;

    const double top = near * std::tan(fovY / 2.0);
    const double right = top * aspect;

    m_camera->setProjection(
        filament::Camera::Projection::PERSPECTIVE,
        -right,
        right,
        -top,
        top,
        near,
        far
    );

    m_cameraTarget = filament::math::double3{0.0, -0.5, -0.5};

    m_camera->lookAt(
        filament::math::double3{0.0, 2.0, 3.5},
        m_cameraTarget,
        filament::math::double3{0.0, 1.0, 0.0}
    );

    // -----------------------------------------------------
    // Configure View
    // -----------------------------------------------------

    m_view->setScene(m_scene);
    m_view->setCamera(m_camera);
    m_view->setFrustumCullingEnabled(false);


    m_view->setViewport({
        0,
        0,
        1280,
        720
    });

    filament::Renderer::ClearOptions clearOptions{};
    clearOptions.clear = true;
    clearOptions.clearColor = { 0.1f, 0.2f, 0.4f, 1.0f };

    m_renderer->setClearOptions(clearOptions);

    return true;
}

filament::math::float3 cross(const filament::math::float3& a, const filament::math::float3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

filament::math::float3 FilamentRenderer::getCameraPos() {
    return m_camera->getPosition();
}
filament::math::float3 FilamentRenderer::getCameraTarget() {
    return m_cameraTarget;
}
filament::math::float3 FilamentRenderer::getCameraUp() {
    return m_camera->getUpVector();
}
filament::math::float3 FilamentRenderer::getCameraLeft() {
    return m_camera->getLeftVector();
}
void FilamentRenderer::setCameraPos(filament::math::float3 pos) {
    m_camera->lookAt(
        pos,
        m_cameraTarget,
        m_camera->getUpVector()
    );
}
void FilamentRenderer::setCameraTarget(filament::math::float3 tar) {
    m_cameraTarget = tar;
    m_camera->lookAt(
        m_camera->getPosition(),
        m_cameraTarget,
        m_camera->getUpVector()
    );
}
void FilamentRenderer::setCameraUp(filament::math::float3 up) {
    m_camera->lookAt(
        m_camera->getPosition(),
        m_cameraTarget,
        up
    );
}

void FilamentRenderer::renderFrame(const uint32_t x, const uint32_t y) {

    if (!m_renderer || !m_swapChain) {
        return;
    }

    // reset viewport
    if (m_lastScreenSize.x != x || m_lastScreenSize.y != y) {
        m_view->setViewport({
            0,
            0,
            x,
            y
        });
        m_lastScreenSize.x = x;
        m_lastScreenSize.y = y;

        // reset camera aspect
        const double aspect = (double)m_lastScreenSize.x / (double)m_lastScreenSize.y;
        const double fovY = 45.0;
    
        m_camera->setProjection(fovY, aspect, m_camera->getNear(), m_camera->getCullingFar());
    }


    // Render one frame.
    if (m_renderer->beginFrame(m_swapChain)) {

        m_renderer->render(m_view);

        m_renderer->endFrame();
    }
}

void FilamentRenderer::shutdown() {

    if (!m_engine) {
        return;
    }

    // models
    for (auto& model : m_models) {
        if (model && m_scene) {
            m_modelLoader.unload(model, m_scene);
            model = nullptr;
        }
    }

    // entities
    if (m_dirLightEntity) {
        m_scene->remove(m_dirLightEntity);
        m_engine->destroy(m_dirLightEntity);
        m_dirLightEntity = {};
    }

    // -----------------------------------------------------
    // Camera component
    // -----------------------------------------------------

    if (m_cameraCreated) {

        m_engine->destroyCameraComponent(
            m_cameraEntity
        );

        m_engine->getEntityManager().destroy(
            m_cameraEntity
        );

        m_camera = nullptr;
        m_cameraEntity = {};
        m_cameraCreated = false;
    }

    // -----------------------------------------------------
    // Engine-managed objects
    // -----------------------------------------------------

    if (m_view) {
        m_engine->destroy(m_view);
        m_view = nullptr;
    }

    if (m_scene) {
        m_engine->destroy(m_scene);
        m_scene = nullptr;
    }

    if (m_renderer) {
        m_engine->destroy(m_renderer);
        m_renderer = nullptr;
    }

    if (m_swapChain) {
        m_engine->destroy(m_swapChain);
        m_swapChain = nullptr;
    }

    // Model Loader
    m_modelLoader.shutdown();

    // -----------------------------------------------------
    // Engine last
    // -----------------------------------------------------

    filament::Engine::destroy(&m_engine);

    m_engine = nullptr;
}
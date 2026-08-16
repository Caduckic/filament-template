#include "filament_renderer.h"

#include <utils/EntityManager.h>

#include <fstream>
#include <vector>

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

    m_lightEntity = em.create();

    filament::LightManager::Builder(
        filament::LightManager::Type::DIRECTIONAL
    )
        .color({1.0f, 1.0f, 1.0f})
        .intensity(100000.0f)
        .direction({-0.5f, -1.0f, -0.5f})
        .castShadows(true)
        .build(
            *m_engine,
            m_lightEntity
        );

    m_scene->addEntity(m_lightEntity);

    // m_fillLight = em.create();

    // filament::LightManager::Builder(
    //     filament::LightManager::Type::DIRECTIONAL
    // )
    //     .color({0.7f, 0.8f, 1.0f})
    //     .intensity(20000.0f)
    //     .direction({0.5f, -0.5f, 0.5f})
    //     .castShadows(false)
    //     .build(*m_engine, m_fillLight);

    // m_scene->addEntity(m_fillLight);
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

    m_indirectLight =
        filament::IndirectLight::Builder()
            .irradiance(3, sh)
            .intensity(30000.0f)
            .build(*m_engine);

    if (!m_indirectLight) {
        shutdown();
        // return false;
    }

    m_scene->setIndirectLight(m_indirectLight);
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
      m_triangleEntity{},
      m_vertexBuffer(nullptr),
      m_indexBuffer(nullptr),
      m_material(nullptr),
      m_materialInstance(nullptr),
      m_model(nullptr) {
}

FilamentRenderer::~FilamentRenderer() {
    shutdown();
}

bool FilamentRenderer::initialize(void* nativeWindow) {
    m_lastFrame = std::chrono::steady_clock::now();

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
    // m_swapChain = m_engine->createSwapChain(1280, 720);

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

    m_model = m_modelLoader.load(
        "assets/models/Insert Cabinet.glb",
        m_scene
    );

    std::printf("INIT: model = %p\n", (void*)m_model);

    if (!m_model) {
        shutdown();
        return false;
    }

    auto& tm = m_engine->getTransformManager();

    auto root = m_model->getRoot();
    auto rootInstance = tm.getInstance(root);

    if (rootInstance.isValid()) {
        tm.setTransform(
            rootInstance,
            filament::math::mat4f::translation(
                filament::math::float3{0.0f, -1.0f, 0.0f}
            )
        );
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

    const double aspect = 1280.0 / 720.0;
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

    m_camera->lookAt(
        filament::math::double3{0.0, 0.0, 3.0},
        filament::math::double3{0.0, 0.0, 0.0},
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
    
    // -----------------------------------------------------
    // Cube geometry
    // -----------------------------------------------------

    static const Vertex vertices[] = {

        // Front (+Z) — RED
        {{-1.0f, -1.0f,  1.0f}, {1, 0, 0, 1}},
        {{ 1.0f, -1.0f,  1.0f}, {1, 0, 0, 1}},
        {{ 1.0f,  1.0f,  1.0f}, {1, 0, 0, 1}},
        {{-1.0f,  1.0f,  1.0f}, {1, 0, 0, 1}},

        // Back (-Z) — GREEN
        {{ 1.0f, -1.0f, -1.0f}, {0, 1, 0, 1}},
        {{-1.0f, -1.0f, -1.0f}, {0, 1, 0, 1}},
        {{-1.0f,  1.0f, -1.0f}, {0, 1, 0, 1}},
        {{ 1.0f,  1.0f, -1.0f}, {0, 1, 0, 1}},

        // Left (-X) — BLUE
        {{-1.0f, -1.0f, -1.0f}, {0, 0, 1, 1}},
        {{-1.0f, -1.0f,  1.0f}, {0, 0, 1, 1}},
        {{-1.0f,  1.0f,  1.0f}, {0, 0, 1, 1}},
        {{-1.0f,  1.0f, -1.0f}, {0, 0, 1, 1}},

        // Right (+X) — YELLOW
        {{ 1.0f, -1.0f,  1.0f}, {1, 1, 0, 1}},
        {{ 1.0f, -1.0f, -1.0f}, {1, 1, 0, 1}},
        {{ 1.0f,  1.0f, -1.0f}, {1, 1, 0, 1}},
        {{ 1.0f,  1.0f,  1.0f}, {1, 1, 0, 1}},

        // Top (+Y) — MAGENTA
        {{-1.0f,  1.0f,  1.0f}, {1, 0, 1, 1}},
        {{ 1.0f,  1.0f,  1.0f}, {1, 0, 1, 1}},
        {{ 1.0f,  1.0f, -1.0f}, {1, 0, 1, 1}},
        {{-1.0f,  1.0f, -1.0f}, {1, 0, 1, 1}},

        // Bottom (-Y) — CYAN
        {{-1.0f, -1.0f, -1.0f}, {0, 1, 1, 1}},
        {{ 1.0f, -1.0f, -1.0f}, {0, 1, 1, 1}},
        {{ 1.0f, -1.0f,  1.0f}, {0, 1, 1, 1}},
        {{-1.0f, -1.0f,  1.0f}, {0, 1, 1, 1}},
    };

    static const uint16_t indices[] = {

        // Front
        0, 1, 2,
        0, 2, 3,

        // Back
        4, 5, 6,
        4, 6, 7,

        // Left
        8, 9, 10,
        8, 10, 11,

        // Right
        12, 13, 14,
        12, 14, 15,

        // Top
        16, 17, 18,
        16, 18, 19,

        // Bottom
        20, 21, 22,
        20, 22, 23
    };

    m_vertexBuffer =
        filament::VertexBuffer::Builder()
            .vertexCount(24)
            .bufferCount(1)
            .attribute(
                filament::VertexAttribute::POSITION,
                0,
                filament::VertexBuffer::AttributeType::FLOAT3,
                offsetof(Vertex, position),
                sizeof(Vertex)
            )
            .attribute(
                filament::VertexAttribute::COLOR,
                0,
                filament::VertexBuffer::AttributeType::FLOAT3,
                offsetof(Vertex, color),
                sizeof(Vertex)
            )
            .build(*m_engine);

    if (!m_vertexBuffer) {
        shutdown();
        return false;
    }
    
    m_vertexBuffer->setBufferAt(
        *m_engine,
        0,
        filament::VertexBuffer::BufferDescriptor(
            vertices,
            sizeof(vertices)
        )
    );

    m_indexBuffer =
        filament::IndexBuffer::Builder()
            .indexCount(36)
            .bufferType(filament::IndexBuffer::IndexType::USHORT)
            .build(*m_engine);
    
    if (!m_indexBuffer) {
        shutdown();
        return false;
    }

    m_indexBuffer->setBuffer(
        *m_engine,
        filament::IndexBuffer::BufferDescriptor(
            indices,
            sizeof(indices)
        )
    );

#if defined(MY_PLATFORM_WEB)
    const char* materialPath =
        "/assets/materials/triangle.filamat";
#else
    const char* materialPath =
        "build/linux/assets/materials/triangle.filamat";
#endif

    std::printf(
        "Loading material: %s\n",
        materialPath
    );

    const auto materialData = readFile(materialPath);

    std::printf(
        "Material size: %zu bytes\n",
        materialData.size()
    );

    if (materialData.empty()) {
        std::fprintf(
            stderr,
            "FAILED TO LOAD MATERIAL\n"
        );

        shutdown();
        return false;
    }

    m_material =
        filament::Material::Builder()
            .package(
                materialData.data(),
                materialData.size()
            )
            .build(*m_engine);

    if (!m_material) {
        shutdown();
        return false;
    }

    m_materialInstance =
        m_material->createInstance();

    if (!m_materialInstance) {
        shutdown();
        return false;
    }

    m_triangleEntity =
        m_engine->getEntityManager().create();

    const filament::Box cubeBox{
        { -0.5f, -0.5f, -0.5f },
        {  0.5f,  0.5f,  0.5f }
    };

    auto result =
        filament::RenderableManager::Builder(1)
            .boundingBox(cubeBox)
            .culling(false)
            .castShadows(false)
            .receiveShadows(false)
            .geometry(
                0,
                filament::RenderableManager::PrimitiveType::TRIANGLES,
                m_vertexBuffer,
                m_indexBuffer
            )
            .material(
                0,
                m_materialInstance
            )
            .build(
                *m_engine,
                m_triangleEntity
            );
    
    if (result != filament::RenderableManager::Builder::Result::Success) {
        fprintf(stderr, "Failed to build triangle renderable: %d\n",
                static_cast<int>(result));

        m_engine->getEntityManager().destroy(m_triangleEntity);
        m_triangleEntity = {};

        shutdown();
        return false;
    }
        
    // m_scene->addEntity(m_triangleEntity);

    return true;
}

void FilamentRenderer::renderFrame() {

    const auto now = std::chrono::steady_clock::now();

    const float deltaTime =
        std::chrono::duration<float>(now - m_lastFrame).count();

    m_lastFrame = now;

    if (!m_renderer || !m_swapChain || !m_triangleEntity) {
        return;
    }

    // Rotate the triangle.
    m_rotation += 1.0f * deltaTime;

    if (m_rotation > 6.283185307f) {
        m_rotation -= 6.283185307f;
    }

    auto& transformManager =
        m_engine->getTransformManager();

    auto instance =
        transformManager.getInstance(m_model->getRoot());

    if (!instance) {
        return;
    }

    transformManager.setTransform(
        instance,
        filament::math::mat4f::rotation(
            m_rotation,
            filament::math::float3{1.0f, 1.0f, 0.0f}
        )
    );

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
    if (m_model && m_scene) {
        m_modelLoader.unload(m_model, m_scene);
        m_model = nullptr;
    }

    // entities
    if (m_triangleEntity) {
        m_scene->remove(m_triangleEntity);

        m_engine->destroy(m_triangleEntity);
        m_triangleEntity = {};
    }

    if (m_materialInstance) {
        m_engine->destroy(m_materialInstance);
        m_materialInstance = nullptr;
    }

    if (m_material) {
        m_engine->destroy(m_material);
        m_material = nullptr;
    }

    if (m_indexBuffer) {
        m_engine->destroy(m_indexBuffer);
        m_indexBuffer = nullptr;
    }

    if (m_vertexBuffer) {
        m_engine->destroy(m_vertexBuffer);
        m_vertexBuffer = nullptr;
    }

    if (m_lightEntity) {
        m_scene->remove(m_lightEntity);
        m_engine->destroy(m_lightEntity);
        m_lightEntity = {};
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
#pragma once

#include "../geometry/Geometry.h"
#include "../geometry/GeometryGenerator.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <string>
#include "../vulkan/UniformBufferObject.h"
#include "ParticleSystem.h"

struct OrbitData {
    bool isOrbiting = false;
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 1.0f;
    float speed = 1.0f; // radians per second
    glm::vec3 axis = glm::vec3(0.0f, 1.0f, 0.0f); // Normalized orbit axis
    float initialAngle = 0.0f; // Initial angle offset (in radians)
    float currentAngle = 0.0f; // Internal state: current angle (in radians)
};

struct SceneLight {
    std::string name;
    Light vulkanLight;
    OrbitData orbitData;
};

struct SceneObject {
    std::string name;
    std::unique_ptr<Geometry> geometry;
    glm::mat4 transform = glm::mat4(1.0f);
    bool visible = true;
    std::string texturePath;
    int shadingMode = 1; // 0=Gouraud, 1=Phong (Default)

    // New: whether this object contributes to shadow map generation
    bool castsShadow = true;

    OrbitData orbitData;

    SceneObject(std::unique_ptr<Geometry> geo, const std::string& texPath = "", const std::string& objName = "")
        : geometry(std::move(geo)), texturePath(texPath), name(objName) {
    }
};

class Scene {
public:
    Scene(VkDevice device, VkPhysicalDevice physicalDevice);
    ~Scene();

    void AddTriangle(const std::string& name, const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddQuad(const std::string& name, const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddCircle(const std::string& name, int segments = 32, float radius = 0.5f, const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    // AddCube now accepts optional scale
    void AddCube(const std::string& name, const glm::vec3& position = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f), const std::string& texturePath = "");
    void AddGrid(const std::string& name, int rows, int cols, float cellSize = 0.1f, const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddSphere(const std::string& name, int stacks = 16, int slices = 32, float radius = 0.5f, const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddModel(const std::string& name, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const std::string& modelPath, const std::string& texturePath);
    void AddGeometry(const std::string& name, std::unique_ptr<Geometry> geometry, const glm::vec3& position = glm::vec3(0.0f));

    void AddLight(const std::string& name, const glm::vec3& position, const glm::vec3& color, float intensity, int type);

    void SetObjectOrbit(const std::string& name, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad = 0.0f);
    void SetLightOrbit(const std::string& name, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad = 0.0f);

    void SetupParticleSystem(VkCommandPool commandPool, VkQueue graphicsQueue,
        GraphicsPipeline* additivePipeline, GraphicsPipeline* alphaPipeline,
        VkDescriptorSetLayout layout, uint32_t framesInFlight);

    // Particle Methods
    void AddFire(const glm::vec3& position, float scale, bool createSmoke);
    void AddSmoke(const glm::vec3& position, float scale);
    void AddRain();
    void AddSnow();
    void AddDust();

    // Accessors for Renderer
    const std::vector<std::unique_ptr<ParticleSystem>>& GetParticleSystems() const { return particleSystems; }

    void Update(float deltaTime);

    const std::vector<Light> GetLights() const;

    // Scene management
    void Clear();
    const std::vector<std::unique_ptr<SceneObject>>& GetObjects() const { return objects; }

    // Transform / visibility helpers
    void SetObjectTransform(size_t index, const glm::mat4& transform);
    void SetObjectVisible(size_t index, bool visible);
    void SetOrbitSpeed(const std::string& name, float speedRadPerSec);

    // New: enable/disable shadow casting for named object
    void SetObjectCastsShadow(const std::string& name, bool casts);

    void SetObjectShadingMode(const std::string& name, int mode);

    void Cleanup();

private:
    void AddObjectInternal(const std::string& name, std::unique_ptr<Geometry> geometry, const glm::vec3& position, const std::string& texturePath);
    glm::vec3 InitializeOrbit(OrbitData& data, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad);


    std::vector<SceneLight> m_SceneLights;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    std::vector<std::unique_ptr<SceneObject>> objects;

    ParticleSystem* GetOrCreateSystem(const ParticleProps& props);

    // Particle Resources
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    GraphicsPipeline* particlePipelineAdditive = nullptr;
    GraphicsPipeline* particlePipelineAlpha = nullptr;
    VkDescriptorSetLayout particleDescriptorLayout = VK_NULL_HANDLE;
    uint32_t framesInFlight = 2;

    std::vector<std::unique_ptr<ParticleSystem>> particleSystems;
};
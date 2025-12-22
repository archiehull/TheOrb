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

namespace SceneLayers {
    // Bit 0: Objects inside the crystal ball (Terrain, Trees)
    constexpr int INSIDE = 1 << 0;  // Value: 1

    // Bit 1: Objects outside the ball (Pedestal, Room)
    constexpr int OUTSIDE = 1 << 1; // Value: 2

    // Helper for objects visible everywhere (Sun, Moon)
    // Value: 3 (Binary 0011)
    constexpr int ALL = INSIDE | OUTSIDE;
}

struct SceneLight {
    std::string name;
    Light vulkanLight;
    OrbitData orbitData;
    int layerMask = SceneLayers::INSIDE;
};



struct SceneObject {
    std::string name;
    std::unique_ptr<Geometry> geometry;
    glm::mat4 transform = glm::mat4(1.0f);
    bool visible = true;
    std::string texturePath;
    int shadingMode = 1; // 0=Gouraud, 1=Phong (Default)

    bool castsShadow = true;
    bool receiveShadows = true;

    OrbitData orbitData;

    int layerMask = SceneLayers::INSIDE;

    explicit SceneObject(std::unique_ptr<Geometry> geo, const std::string& texPath = "", const std::string& objName = "")
        : name(objName), geometry(std::move(geo)), texturePath(texPath) {
    }
};

struct ProceduralObjectConfig {
    std::string modelPath;
    std::string texturePath;
    float frequency;
    glm::vec3 minScale;
    glm::vec3 maxScale;
    glm::vec3 baseRotation;
};

class Scene final {
public:
    Scene(VkDevice vkDevice, VkPhysicalDevice physDevice);
    ~Scene() = default;

    // Non-copyable
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    float RadiusAdjustment(const float radius, const float deltaY) const;

    void AddTerrain(const std::string& name, float radius, int rings, int segments, float heightScale, float noiseFreq, const glm::vec3& position, const std::string& texturePath);

    void AddCube(const std::string& name, const glm::vec3& position = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f), const std::string& texturePath = "");
    void AddGrid(const std::string& name, int rows, int cols, float cellSize = 0.1f, const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddSphere(const std::string& name, int stacks = 16, int slices = 32, float radius = 0.5f, const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddModel(const std::string& name, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const std::string& modelPath, const std::string& texturePath);
    void AddGeometry(const std::string& name, std::unique_ptr<Geometry> geometry, const glm::vec3& position = glm::vec3(0.0f));

    void AddLight(const std::string& name, const glm::vec3& position, const glm::vec3& color, float intensity, int type);

    void SetObjectOrbit(const std::string& name, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad = 0.0f);
    void SetLightOrbit(const std::string& name, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad = 0.0f);

    void AddBowl(const std::string& name, float radius, int slices, int stacks, const glm::vec3& position, const std::string& texturePath);
    void AddPedestal(const std::string& name, float topRadius, float baseWidth, float height, const glm::vec3& position, const std::string& texturePath);

    void SetupParticleSystem(VkCommandPool commandPool, VkQueue graphicsQueue,
        GraphicsPipeline* additivePipeline, GraphicsPipeline* alphaPipeline,
        VkDescriptorSetLayout layout, uint32_t framesInFlight);

    // Procedural Generation API
    void RegisterProceduralObject(const std::string& modelPath, const std::string& texturePath, float frequency, const glm::vec3& minScale, const glm::vec3& maxScale, const glm::vec3& baseRotation = glm::vec3(0.0f));
    void GenerateProceduralObjects(int count, float terrainRadius, float deltaY, float heightScale, float noiseFreq);

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

    void SetObjectLayerMask(const std::string& name, int mask);
    void SetLightLayerMask(const std::string& name, int mask);

    void SetObjectCastsShadow(const std::string& name, bool casts);
    void SetObjectReceivesShadows(const std::string& name, bool receives);
    void SetObjectShadingMode(const std::string& name, int mode);

    void Cleanup() { Clear(); }

private:
    void AddObjectInternal(const std::string& name, std::unique_ptr<Geometry> geometry, const glm::vec3& position, const std::string& texturePath);

    glm::vec3 InitializeOrbit(OrbitData& data, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad) const;

    std::vector<SceneLight> m_SceneLights;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    std::vector<std::unique_ptr<SceneObject>> objects;
    std::vector<ProceduralObjectConfig> proceduralRegistry;

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
#pragma once

#include "../geometry/Geometry.h"
#include "../geometry/GeometryGenerator.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <string>

struct SceneObject {
    std::unique_ptr<Geometry> geometry;
    glm::mat4 transform = glm::mat4(1.0f);
    bool visible = true;
    std::string texturePath;
    int shadingMode = 1; // 0=Gouraud, 1=Phong (Default)

    SceneObject(std::unique_ptr<Geometry> geo, const std::string& texPath = "")
        : geometry(std::move(geo)), texturePath(texPath) {
    }
};

class Scene {
public:
    Scene(VkDevice device, VkPhysicalDevice physicalDevice);
    ~Scene();

    // Easy API to add shapes to your scene (now accept a position)
    void AddTriangle(const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddQuad(const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddCircle(int segments = 32, float radius = 0.5f, const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddCube(const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddGrid(int rows, int cols, float cellSize = 0.1f, const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
	void AddSphere(int stacks = 16, int slices = 32, float radius = 0.5f, const glm::vec3& position = glm::vec3(0.0f), const std::string& texturePath = "");
    void AddModel(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const std::string& modelPath, const std::string& texturePath);
    void AddGeometry(std::unique_ptr<Geometry> geometry, const glm::vec3& position = glm::vec3(0.0f));

    // Scene management
    void Clear();
    const std::vector<std::unique_ptr<SceneObject>>& GetObjects() const { return objects; }

    // Transform / visibility helpers
    void SetObjectTransform(size_t index, const glm::mat4& transform);
    void SetObjectVisible(size_t index, bool visible);

    void Cleanup();

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    std::vector<std::unique_ptr<SceneObject>> objects;
};
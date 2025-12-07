#pragma once

#include "../geometry/Geometry.h"
#include "../geometry/GeometryGenerator.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

// Represents a drawable object in the scene
struct SceneObject {
    std::unique_ptr<Geometry> geometry;
    glm::mat4 transform = glm::mat4(1.0f);  // Future: position, rotation, scale
    bool visible = true;

    SceneObject(std::unique_ptr<Geometry> geo) : geometry(std::move(geo)) {}
};

class Scene {
public:
    Scene(VkDevice device, VkPhysicalDevice physicalDevice);
    ~Scene();

    // Easy API to add shapes to your scene
    void AddTriangle();
    void AddQuad();
    void AddCircle(int segments = 32, float radius = 0.5f);
    void AddCube();
    void AddGrid(int rows, int cols, float cellSize = 0.1f);

    // Custom geometry
    void AddGeometry(std::unique_ptr<Geometry> geometry);

    // Scene management
    void Clear();
    const std::vector<std::unique_ptr<SceneObject>>& GetObjects() const { return objects; }

    // Future: Transform objects
    void SetObjectTransform(size_t index, const glm::mat4& transform);
    void SetObjectVisible(size_t index, bool visible);

    void Cleanup();

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    std::vector<std::unique_ptr<SceneObject>> objects;
};
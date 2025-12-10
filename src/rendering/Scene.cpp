#include "Scene.h"
#include "../geometry/OBJLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

static void UpdateShadingMode(SceneObject* obj) {
    if (!obj || !obj->geometry) return;

    const size_t HIGH_POLY_THRESHOLD = 500;
    size_t vertexCount = obj->geometry->GetVertices().size();

    if (vertexCount > HIGH_POLY_THRESHOLD) {
        obj->shadingMode = 0; // Gouraud
        // std::cout << "Auto-Shading: High Poly (" << vertexCount << "v) -> Gouraud" << std::endl;
    }
    else {
        obj->shadingMode = 1; // Phong
        // std::cout << "Auto-Shading: Low Poly (" << vertexCount << "v) -> Phong" << std::endl;
    }
}

Scene::Scene(VkDevice device, VkPhysicalDevice physicalDevice)
    : device(device), physicalDevice(physicalDevice) {
}

Scene::~Scene() {
}

void Scene::AddTriangle(const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateTriangle(device, physicalDevice);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);

    UpdateShadingMode(obj.get()); 

    objects.push_back(std::move(obj));
}

void Scene::AddQuad(const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateQuad(device, physicalDevice);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);

    UpdateShadingMode(obj.get()); 

    objects.push_back(std::move(obj));
}

void Scene::AddCircle(int segments, float radius, const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateCircle(device, physicalDevice, segments, radius);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);

    UpdateShadingMode(obj.get()); 

    objects.push_back(std::move(obj));
}

void Scene::AddCube(const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateCube(device, physicalDevice);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);

    UpdateShadingMode(obj.get()); 

    objects.push_back(std::move(obj));
}

void Scene::AddGrid(int rows, int cols, float cellSize, const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateGrid(device, physicalDevice, rows, cols, cellSize);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);

    UpdateShadingMode(obj.get()); 

    objects.push_back(std::move(obj));
}

void Scene::AddSphere(int stacks, int slices, float radius, const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateSphere(device, physicalDevice, stacks, slices, radius);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);

    UpdateShadingMode(obj.get()); 

    objects.push_back(std::move(obj));
}

void Scene::AddModel(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const std::string& modelPath, const std::string& texturePath) {
    try {
        auto geometry = OBJLoader::Load(device, physicalDevice, modelPath);
        auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::scale(transform, scale);

        obj->transform = transform;

        UpdateShadingMode(obj.get()); 

        objects.push_back(std::move(obj));
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to add model '" << modelPath << "': " << e.what() << std::endl;
    }
}

void Scene::AddLight(const glm::vec3& position, const glm::vec3& color, float intensity, int type) {
    if (m_Lights.size() >= MAX_LIGHTS) {
        std::cerr << "Warning: Maximum number of lights (" << MAX_LIGHTS << ") reached. Light not added." << std::endl;
        return;
    }

    Light newLight{};
    newLight.position = position;
    newLight.color = color;
    newLight.intensity = intensity;
    newLight.type = type;

    m_Lights.push_back(newLight);
}

void Scene::AddGeometry(std::unique_ptr<Geometry> geometry, const glm::vec3& position) {
    auto obj = std::make_unique<SceneObject>(std::move(geometry));
    obj->transform = glm::translate(glm::mat4(1.0f), position);

    UpdateShadingMode(obj.get()); 

    objects.push_back(std::move(obj));
}

void Scene::Clear() {
    for (auto& obj : objects) {
        if (obj && obj->geometry) {
            obj->geometry->Cleanup();
        }
    }
    objects.clear();
}

void Scene::SetObjectTransform(size_t index, const glm::mat4& transform) {
    if (index < objects.size()) {
        objects[index]->transform = transform;
    }
}

void Scene::SetObjectVisible(size_t index, bool visible) {
    if (index < objects.size()) {
        objects[index]->visible = visible;
    }
}

void Scene::Cleanup() {
    Clear();
}
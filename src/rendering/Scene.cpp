#include "Scene.h"
#include "../geometry/OBJLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Scene::Scene(VkDevice device, VkPhysicalDevice physicalDevice)
    : device(device), physicalDevice(physicalDevice) {
}

Scene::~Scene() {
}

void Scene::AddTriangle(const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateTriangle(device, physicalDevice);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
}

void Scene::AddQuad(const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateQuad(device, physicalDevice);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
}

void Scene::AddCircle(int segments, float radius, const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateCircle(device, physicalDevice, segments, radius);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
}

void Scene::AddCube(const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateCube(device, physicalDevice);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
}

void Scene::AddGrid(int rows, int cols, float cellSize, const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateGrid(device, physicalDevice, rows, cols, cellSize);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
}

void Scene::AddSphere(int stacks, int slices, float radius, const glm::vec3& position, const std::string& texturePath) {
    auto geometry = GeometryGenerator::CreateSphere(device, physicalDevice, stacks, slices, radius);
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
}

void Scene::AddModel(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const std::string& modelPath, const std::string& texturePath) {
    try {
        auto geometry = OBJLoader::Load(device, physicalDevice, modelPath);
        auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath);

        glm::mat4 transform = glm::mat4(1.0f);

        // 1. Translate
        transform = glm::translate(transform, position);

        // 2. Rotate (Applied in order: X, then Y, then Z)
        // We convert degrees to radians because glm expects radians
        transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        // 3. Scale
        transform = glm::scale(transform, scale);

        obj->transform = transform;
        objects.push_back(std::move(obj));
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to add model '" << modelPath << "': " << e.what() << std::endl;
    }
}

void Scene::AddGeometry(std::unique_ptr<Geometry> geometry, const glm::vec3& position) {
    auto obj = std::make_unique<SceneObject>(std::move(geometry));
    obj->transform = glm::translate(glm::mat4(1.0f), position);
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
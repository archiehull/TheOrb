#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>

Scene::Scene(VkDevice device, VkPhysicalDevice physicalDevice)
    : device(device), physicalDevice(physicalDevice) {
}

Scene::~Scene() {
}

void Scene::AddTriangle(const glm::vec3& position) {
    auto geometry = GeometryGenerator::CreateTriangle(device, physicalDevice);
    auto obj = std::make_unique<SceneObject>(std::move(geometry));
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
}

void Scene::AddQuad(const glm::vec3& position) {
    auto geometry = GeometryGenerator::CreateQuad(device, physicalDevice);
    auto obj = std::make_unique<SceneObject>(std::move(geometry));
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
}

void Scene::AddCircle(int segments, float radius, const glm::vec3& position) {
    auto geometry = GeometryGenerator::CreateCircle(device, physicalDevice, segments, radius);
    auto obj = std::make_unique<SceneObject>(std::move(geometry));
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
}

void Scene::AddCube(const glm::vec3& position) {
    auto geometry = GeometryGenerator::CreateCube(device, physicalDevice);
    auto obj = std::make_unique<SceneObject>(std::move(geometry));
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
}

void Scene::AddGrid(int rows, int cols, float cellSize, const glm::vec3& position) {
    auto geometry = GeometryGenerator::CreateGrid(device, physicalDevice, rows, cols, cellSize);
    auto obj = std::make_unique<SceneObject>(std::move(geometry));
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    objects.push_back(std::move(obj));
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
#include "Scene.h"

Scene::Scene(VkDevice device, VkPhysicalDevice physicalDevice)
    : device(device), physicalDevice(physicalDevice) {
}

Scene::~Scene() {
}

void Scene::AddTriangle() {
    auto geometry = GeometryGenerator::CreateTriangle(device, physicalDevice);
    objects.push_back(std::make_unique<SceneObject>(std::move(geometry)));
}

void Scene::AddQuad() {
    auto geometry = GeometryGenerator::CreateQuad(device, physicalDevice);
    objects.push_back(std::make_unique<SceneObject>(std::move(geometry)));
}

void Scene::AddCircle(int segments, float radius) {
    auto geometry = GeometryGenerator::CreateCircle(device, physicalDevice, segments, radius);
    objects.push_back(std::make_unique<SceneObject>(std::move(geometry)));
}

void Scene::AddCube() {
    auto geometry = GeometryGenerator::CreateCube(device, physicalDevice);
    objects.push_back(std::make_unique<SceneObject>(std::move(geometry)));
}

void Scene::AddGrid(int rows, int cols, float cellSize) {
    auto geometry = GeometryGenerator::CreateGrid(device, physicalDevice, rows, cols, cellSize);
    objects.push_back(std::make_unique<SceneObject>(std::move(geometry)));
}

void Scene::AddGeometry(std::unique_ptr<Geometry> geometry) {
    objects.push_back(std::make_unique<SceneObject>(std::move(geometry)));
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
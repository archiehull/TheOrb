#include "Scene.h"
#include "../geometry/OBJLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/common.hpp>
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

void Scene::AddObjectInternal(const std::string& name, std::unique_ptr<Geometry> geometry, const glm::vec3& position, const std::string& texturePath) {
    auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath, name);
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    UpdateShadingMode(obj.get());
    objects.push_back(std::move(obj));
}

Scene::Scene(VkDevice device, VkPhysicalDevice physicalDevice)
    : device(device), physicalDevice(physicalDevice) {
}

Scene::~Scene() {
}

void Scene::AddTriangle(const std::string& name, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateTriangle(device, physicalDevice), position, texturePath);
}

void Scene::AddQuad(const std::string& name, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateQuad(device, physicalDevice), position, texturePath);
}

void Scene::AddCircle(const std::string& name, int segments, float radius, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateCircle(device, physicalDevice, segments, radius), position, texturePath);
}

void Scene::AddCube(const std::string& name, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateCube(device, physicalDevice), position, texturePath);
}

void Scene::AddGrid(const std::string& name, int rows, int cols, float cellSize, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateGrid(device, physicalDevice, rows, cols, cellSize), position, texturePath);
}

void Scene::AddSphere(const std::string& name, int stacks, int slices, float radius, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateSphere(device, physicalDevice, stacks, slices, radius), position, texturePath);
}

void Scene::AddGeometry(const std::string& name, std::unique_ptr<Geometry> geometry, const glm::vec3& position) {
    AddObjectInternal(name, std::move(geometry), position, "");
}

void Scene::AddModel(const std::string& name, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const std::string& modelPath, const std::string& texturePath) {
    try {
        auto geometry = OBJLoader::Load(device, physicalDevice, modelPath);

        auto obj = std::make_unique<SceneObject>(std::move(geometry), texturePath, name);

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


void Scene::AddLight(const std::string& name, const glm::vec3& position, const glm::vec3& color, float intensity, int type) {
    if (m_SceneLights.size() >= MAX_LIGHTS) {
        std::cerr << "Warning: Maximum number of lights (" << MAX_LIGHTS << ") reached. Light not added." << std::endl;
        return;
    }

    SceneLight newSceneLight{};
    newSceneLight.name = name;
    newSceneLight.vulkanLight.position = position;
    newSceneLight.vulkanLight.color = color;
    newSceneLight.vulkanLight.intensity = intensity;
    newSceneLight.vulkanLight.type = type;

    m_SceneLights.push_back(newSceneLight);
}

glm::vec3 Scene::InitializeOrbit(OrbitData& data, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad) {
    data.isOrbiting = true;
    data.center = center;
    data.radius = radius;
    data.speed = speedRadPerSec;
    // guard against zero-length axis
    float axisLen = glm::length(axis);
    data.axis = (axisLen > 1e-6f) ? glm::normalize(axis) : glm::vec3(0.0f, 1.0f, 0.0f);
    data.initialAngle = initialAngleRad;
    data.currentAngle = initialAngleRad;

    // compute initial offset using quaternion (faster / clearer than building a 4x4 rotation matrix)
    glm::quat rot = glm::angleAxis(data.initialAngle, data.axis);
    glm::vec3 offset = rot * glm::vec3(data.radius, 0.0f, 0.0f);
    return data.center + offset;
}

void Scene::SetObjectOrbit(const std::string& name, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad) {

    auto it = std::find_if(objects.begin(), objects.end(),
        [&name](const std::unique_ptr<SceneObject>& obj) {
            return obj->name == name;
        });

    if (it != objects.end()) {
        SceneObject* objectPtr = it->get();
        glm::vec3 initialPosition = InitializeOrbit(objectPtr->orbitData, center, radius, speedRadPerSec, axis, initialAngleRad);

        objectPtr->transform[3] = glm::vec4(initialPosition, 1.0f);
    }
    else {
        std::cerr << "Error: Scene object with name '" << name << "' not found for orbit assignment." << std::endl;
    }
}

void Scene::SetLightOrbit(const std::string& name, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad) {
    // Find the light by name
    auto it = std::find_if(m_SceneLights.begin(), m_SceneLights.end(),
        [&name](const SceneLight& light) {
            return light.name == name;
        });

    if (it != m_SceneLights.end()) {
        SceneLight& sceneLight = *it;
        glm::vec3 initialPosition = InitializeOrbit(sceneLight.orbitData, center, radius, speedRadPerSec, axis, initialAngleRad);
        sceneLight.vulkanLight.position = initialPosition;
    }
    else {
        std::cerr << "Error: Scene light with name '" << name << "' not found for orbit assignment." << std::endl;
    }
}

void Scene::SetOrbitSpeed(const std::string& name, float speedRadPerSec) {
    auto itObj = std::find_if(objects.begin(), objects.end(),
        [&](const std::unique_ptr<SceneObject>& obj) { return obj->name == name; });

    if (itObj != objects.end()) {
        (*itObj)->orbitData.speed = speedRadPerSec;
    }

    auto itLight = std::find_if(m_SceneLights.begin(), m_SceneLights.end(),
        [&](const SceneLight& light) { return light.name == name; });

    if (itLight != m_SceneLights.end()) {
        itLight->orbitData.speed = speedRadPerSec;
    }
}

void Scene::Update(float deltaTime) {

    auto CalculateNewPos = [&](OrbitData& data) -> glm::vec3 {
        data.currentAngle += data.speed * deltaTime;
        glm::quat rotation = glm::angleAxis(data.currentAngle, data.axis);
        glm::vec3 offset = rotation * glm::vec3(data.radius, 0.0f, 0.0f);
        return data.center + offset;
        };

    // --- Light Orbit Update ---
    for (auto& sceneLight : m_SceneLights) {
        if (sceneLight.orbitData.isOrbiting) {
            sceneLight.vulkanLight.position = CalculateNewPos(sceneLight.orbitData);
            /*if (sceneLight.name == "SunLight") {
                const auto& p = sceneLight.vulkanLight.position;
                std::cout << "SunLight pos = (" << p.x << "," << p.y << "," << p.z << ")\n";
            }*/
        }
    }

    // --- Object Orbit Update ---
    for (auto& obj : objects) {
        if (obj->orbitData.isOrbiting) {
            glm::vec3 newPos = CalculateNewPos(obj->orbitData);
            obj->transform[3] = glm::vec4(newPos, 1.0f);
            /*if (obj->name == "Sun") {
                std::cout << "Sun pos = (" << newPos.x << "," << newPos.y << "," << newPos.z << ") angle="
                    << obj->orbitData.currentAngle << "\n";
            }*/
        }
    }
}


// Extracts the Light structs from the SceneLight wrappers
const std::vector<Light> Scene::GetLights() const {
    std::vector<Light> lights;
    lights.reserve(m_SceneLights.size());
    for (const auto& sceneLight : m_SceneLights) {
        lights.push_back(sceneLight.vulkanLight);
    }
    return lights;
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
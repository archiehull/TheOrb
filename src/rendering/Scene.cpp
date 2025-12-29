#include "Scene.h"
#include "ParticleLibrary.h"
#include "../geometry/OBJLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/common.hpp>
#include <iostream>
#include <algorithm>
#include <random>

static void UpdateShadingMode(SceneObject* obj) {
    if (!obj || !obj->geometry) return;

    const size_t HIGH_POLY_THRESHOLD = 500;
    const size_t vertexCount = obj->geometry->GetVertices().size();

    if (vertexCount > HIGH_POLY_THRESHOLD) {
        obj->shadingMode = 0; // Gouraud
    }
    else {
        obj->shadingMode = 1; // Phong
    }
}

void Scene::AddObjectInternal(const std::string& name, std::unique_ptr<Geometry> geometry, const glm::vec3& position, const std::string& texturePath, bool isFlammable) {
    std::shared_ptr<Geometry> sharedGeo = std::move(geometry);
    auto obj = std::make_unique<SceneObject>(sharedGeo, texturePath, name);
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    obj->isFlammable = isFlammable;
    UpdateShadingMode(obj.get());
    objects.push_back(std::move(obj));
}

float Scene::RadiusAdjustment(const float radius, const float deltaY) const {
    const float planeY = deltaY;
    float terrainRadius = 0.0f;
    const float absDist = std::fabs(planeY);
    if (absDist < radius) {
        terrainRadius = std::sqrt(radius * radius - absDist * absDist);
    }
    else {
        terrainRadius = 0.0f; // plane is outside sphere — no intersection
    }
    return terrainRadius;
}

Scene::Scene(VkDevice vkDevice, VkPhysicalDevice physDevice)
    : device(vkDevice), physicalDevice(physDevice) {
    try {
        auto dustGeo = OBJLoader::Load(device, physicalDevice, "models/dust.obj");
        dustGeometryPrototype = std::shared_ptr<Geometry>(std::move(dustGeo));
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load dust prototype: " << e.what() << std::endl;
    }
}

// Destructor implementation removed (now = default in header)

void Scene::RegisterProceduralObject(const std::string& modelPath, const std::string& texturePath, float frequency, const glm::vec3& minScale, const glm::vec3& maxScale, const glm::vec3& baseRotation, bool isFlammable) {
    ProceduralObjectConfig config;
    config.modelPath = modelPath;
    config.texturePath = texturePath;
    config.frequency = frequency;
    config.minScale = minScale;
    config.maxScale = maxScale;
    config.baseRotation = baseRotation;
    config.isFlammable = isFlammable;
    proceduralRegistry.push_back(config);
}

void Scene::GenerateProceduralObjects(int count, float terrainRadius, float deltaY, float heightScale, float noiseFreq) {
    if (proceduralRegistry.empty()) return;

    float totalFreq = 0.0f;
    for (const auto& item : proceduralRegistry) totalFreq += item.frequency;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> distAngle(0.0f, glm::two_pi<float>());
    std::uniform_real_distribution<float> distFreq(0.0f, totalFreq);
    std::uniform_real_distribution<float> distScale(0.0f, 1.0f);
    std::uniform_real_distribution<float> distRot(0.0f, 360.0f);

    for (int i = 0; i < count; i++) {
        // 1. Pick Position
        const float r = std::sqrt(distScale(gen)) * (terrainRadius * 0.9f);
        const float theta = distAngle(gen);
        const float x = r * cos(theta);
        const float z = r * sin(theta);

        // 2. Calculate Height
        const float yOffset = GeometryGenerator::GetTerrainHeight(x, z, terrainRadius, heightScale, noiseFreq);
        const float y = deltaY + yOffset;

        // 3. Select Object
        const float pick = distFreq(gen);
        float current = 0.0f;
        int selectedIndex = 0;
        for (int k = 0; k < proceduralRegistry.size(); k++) {
            current += proceduralRegistry[k].frequency;
            if (pick <= current) {
                selectedIndex = k;
                break;
            }
        }
        const auto& config = proceduralRegistry[selectedIndex];

        // 4. Randomize Scale
        glm::vec3 scale;
        scale.x = glm::mix(config.minScale.x, config.maxScale.x, distScale(gen));
        scale.y = glm::mix(config.minScale.y, config.maxScale.y, distScale(gen));
        scale.z = glm::mix(config.minScale.z, config.maxScale.z, distScale(gen));

        // 5. Spawn Object (with dummy rotation initially)
        const std::string name = "ProcObj_" + std::to_string(i);
        // We pass 0 rotation here because we will manually overwrite the matrix below
        AddModel(name, glm::vec3(x, y, z), glm::vec3(0.0f), scale, config.modelPath, config.texturePath, config.isFlammable);

        // 6. Overwrite Transform with Correct Rotation Order
        if (!objects.empty()) {
            const auto& obj = objects.back();
            glm::mat4 m = glm::mat4(1.0f);

            // A. Translate to position
            m = glm::translate(m, glm::vec3(x, y, z));

            // B. Apply World Yaw (Random Rotation around Y)
            // This spins the object "in place" relative to the world, keeping it upright
            const float randomYaw = distRot(gen);
            m = glm::rotate(m, glm::radians(randomYaw), glm::vec3(0.0f, 1.0f, 0.0f));

            // C. Apply Base Rotation Correction (e.g. Stand up the cactus)
            // This applies to the model's local axes
            if (glm::length(config.baseRotation) > 0.001f) {
                m = glm::rotate(m, glm::radians(config.baseRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                m = glm::rotate(m, glm::radians(config.baseRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                m = glm::rotate(m, glm::radians(config.baseRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            }

            // D. Scale
            m = glm::scale(m, scale);

            obj->transform = m;
        }
    }
}



void Scene::AddTerrain(const std::string& name, float radius, int rings, int segments, float heightScale, float noiseFreq, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateTerrain(device, physicalDevice, radius - 1, rings, segments, heightScale, noiseFreq), position, texturePath, false);
}

void Scene::AddBowl(const std::string& name, float radius, int slices, int stacks, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateBowl(device, physicalDevice, radius, slices, stacks), position, texturePath, false);
}

void Scene::AddPedestal(const std::string& name, float topRadius, float baseWidth, float height, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreatePedestal(device, physicalDevice, topRadius, baseWidth, height, 512, 512), position, texturePath, false);
}

void Scene::AddCube(const std::string& name, const glm::vec3& position, const glm::vec3& scale, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateCube(device, physicalDevice), position, texturePath, false);

    if (!objects.empty()) {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        t = glm::scale(t, scale);
        objects.back()->transform = t;
        UpdateShadingMode(objects.back().get());
    }
}

void Scene::AddGrid(const std::string& name, int rows, int cols, float cellSize, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateGrid(device, physicalDevice, rows, cols, cellSize), position, texturePath, false);
}

void Scene::AddSphere(const std::string& name, int stacks, int slices, float radius, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateSphere(device, physicalDevice, stacks, slices, radius), position, texturePath, false);
}

void Scene::AddGeometry(const std::string& name, std::unique_ptr<Geometry> geometry, const glm::vec3& position) {
    AddObjectInternal(name, std::move(geometry), position, "", false);
}

void Scene::AddModel(const std::string& name, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const std::string& modelPath, const std::string& texturePath, bool isFlammable) {
    try {
        auto geometry = OBJLoader::Load(device, physicalDevice, modelPath);

        // Explicit shared_ptr conversion
        std::shared_ptr<Geometry> sharedGeo = std::move(geometry);
        auto obj = std::make_unique<SceneObject>(sharedGeo, texturePath, name);

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::scale(transform, scale);

        obj->transform = transform;
        obj->isFlammable = isFlammable;

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

    newSceneLight.vulkanLight.layerMask = SceneLayers::INSIDE;
    newSceneLight.layerMask = SceneLayers::INSIDE;

    m_SceneLights.push_back(newSceneLight);
}

void Scene::SetupParticleSystem(VkCommandPool commandPoolArg, VkQueue graphicsQueueArg,
    GraphicsPipeline* additivePipeline, GraphicsPipeline* alphaPipeline,
    VkDescriptorSetLayout layout, uint32_t framesInFlightArg) {
    this->commandPool = commandPoolArg;
    this->graphicsQueue = graphicsQueueArg;
    this->particlePipelineAdditive = additivePipeline;
    this->particlePipelineAlpha = alphaPipeline;
    this->particleDescriptorLayout = layout;
    this->framesInFlight = framesInFlightArg;

    for (const auto& sys : particleSystems) {
        if (sys->IsAdditive()) {
            sys->SetPipeline(particlePipelineAdditive);
        }
        else {
            sys->SetPipeline(particlePipelineAlpha);
        }
    }
}

ParticleSystem* Scene::GetOrCreateSystem(const ParticleProps& props) {
    // Check if a system with this texture already exists
    for (const auto& sys : particleSystems) {
        if (sys->GetTexturePath() == props.texturePath) {
            return sys.get();
        }
    }

    // Create new system
    auto newSys = std::make_unique<ParticleSystem>(device, physicalDevice, commandPool, graphicsQueue, 2000, framesInFlight);

    GraphicsPipeline* const pipeline = props.isAdditive ? particlePipelineAdditive : particlePipelineAlpha;
    newSys->Initialize(particleDescriptorLayout, pipeline, props.texturePath, props.isAdditive);

    ParticleSystem* const ptr = newSys.get();
    particleSystems.push_back(std::move(newSys));
    return ptr;
}

int Scene::AddFire(const glm::vec3& position, float scale) {
    ParticleProps fire = ParticleLibrary::GetFireProps();
    fire.position = position;
    fire.sizeBegin *= scale;
    fire.sizeEnd *= scale;
    return GetOrCreateSystem(fire)->AddEmitter(fire, 300.0f);
}

int Scene::AddSmoke(const glm::vec3& position, float scale) {
    ParticleProps smoke = ParticleLibrary::GetSmokeProps();
    smoke.position = position;
    smoke.sizeBegin *= scale;
    smoke.sizeEnd *= scale;
    return GetOrCreateSystem(smoke)->AddEmitter(smoke, 100.0f);
}

void Scene::AddRain() {
    ParticleProps rain = ParticleLibrary::GetRainProps();
    // Global effect: Emitter covers a large area high up
    rain.position = glm::vec3(0.0f, 40.0f, 0.0f);
    // Huge variance in X and Z to cover the map
    rain.velocityVariation.x = 80.0f; // Width
    rain.velocityVariation.z = 80.0f; // Depth

    // Set bounds for rain (matches CrystalBall radius)
    auto* const sys = GetOrCreateSystem(rain);
    sys->SetSimulationBounds(glm::vec3(0.0f), 150.0f);
    sys->AddEmitter(rain, 1000.0f); // Heavy rain
}

void Scene::AddSnow() {
    ParticleProps snow = ParticleLibrary::GetSnowProps();
    snow.position = glm::vec3(0.0f, 50.0f, 0.0f);

    // CHANGE: Use Position Variation for area spawning, rather than Velocity Variation
    // This allows them to spawn over a wide area but fall straight down gently
    snow.positionVariation = glm::vec3(100.0f, 0.0f, 100.0f);

    // CHANGE: Reduced velocity variation for gentle drift
    snow.velocityVariation = glm::vec3(1.0f, 0.2f, 1.0f);

    // Set bounds for snow (matches CrystalBall radius)
    auto* const sys = GetOrCreateSystem(snow);
    sys->SetSimulationBounds(glm::vec3(0.0f), 150.0f);
    sys->AddEmitter(snow, 500.0f);
}

void Scene::AddDust() {
    ParticleProps dust = ParticleLibrary::GetDustProps();
    dust.position = glm::vec3(0.0f, 5.0f, 0.0f); // Near ground
    dust.velocityVariation.x = 80.0f;
    dust.velocityVariation.z = 80.0f;
    dust.velocityVariation.y = 10.0f; // Height of dust cloud

    // Set bounds for dust (matches CrystalBall radius)
    auto* const sys = GetOrCreateSystem(dust);
    sys->SetSimulationBounds(glm::vec3(0.0f), 150.0f);
    sys->AddEmitter(dust, 200.0f);
}

glm::vec3 Scene::InitializeOrbit(OrbitData& data, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad) const {
    data.isOrbiting = true;
    data.center = center;
    data.radius = radius;
    data.speed = speedRadPerSec;
    const float axisLen = glm::length(axis);
    data.axis = (axisLen > 1e-6f) ? glm::normalize(axis) : glm::vec3(0.0f, 1.0f, 0.0f);
    data.initialAngle = initialAngleRad;
    data.currentAngle = initialAngleRad;

    const glm::quat rot = glm::angleAxis(data.initialAngle, data.axis);
    const glm::vec3 offset = rot * glm::vec3(data.radius, 0.0f, 0.0f);
    return data.center + offset;
}

void Scene::SetObjectOrbit(const std::string& name, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad) {

    const auto it = std::find_if(objects.begin(), objects.end(),
        [&name](const std::unique_ptr<SceneObject>& obj) {
            return obj->name == name;
        });

    if (it != objects.end()) {
        SceneObject* const objectPtr = it->get();
        const glm::vec3 initialPosition = InitializeOrbit(objectPtr->orbitData, center, radius, speedRadPerSec, axis, initialAngleRad);

        objectPtr->transform[3] = glm::vec4(initialPosition, 1.0f);
    }
    else {
        std::cerr << "Error: Scene object with name '" << name << "' not found for orbit assignment." << std::endl;
    }
}

void Scene::SetLightOrbit(const std::string& name, const glm::vec3& center, float radius, float speedRadPerSec, const glm::vec3& axis, float initialAngleRad) {
    const auto it = std::find_if(m_SceneLights.begin(), m_SceneLights.end(),
        [&name](const SceneLight& light) {
            return light.name == name;
        });

    if (it != m_SceneLights.end()) {
        SceneLight& sceneLight = const_cast<SceneLight&>(*it);
        const glm::vec3 initialPosition = InitializeOrbit(sceneLight.orbitData, center, radius, speedRadPerSec, axis, initialAngleRad);
        sceneLight.vulkanLight.position = initialPosition;
    }
    else {
        std::cerr << "Error: Scene light with name '" << name << "' not found for orbit assignment." << std::endl;
    }
}

void Scene::SetOrbitSpeed(const std::string& name, float speedRadPerSec) {
    const auto itObj = std::find_if(objects.begin(), objects.end(),
        [&](const std::unique_ptr<SceneObject>& obj) { return obj->name == name; });

    if (itObj != objects.end()) {
        (*itObj)->orbitData.speed = speedRadPerSec;
    }

    const auto itLight = std::find_if(m_SceneLights.begin(), m_SceneLights.end(),
        [&](const SceneLight& light) { return light.name == name; });

    if (itLight != m_SceneLights.end()) {
        const_cast<SceneLight&>(*itLight).orbitData.speed = speedRadPerSec;
    }
}

void Scene::Update(float deltaTime) {

    auto CalculateNewPos = [&](OrbitData& data) -> glm::vec3 {
        data.currentAngle += data.speed * deltaTime;
        glm::quat rotation = glm::angleAxis(data.currentAngle, data.axis);
        glm::vec3 offset = rotation * glm::vec3(data.radius, 0.0f, 0.0f);
        return data.center + offset;
        };

    for (auto& sceneLight : m_SceneLights) {
        if (sceneLight.orbitData.isOrbiting) {
            sceneLight.vulkanLight.position = CalculateNewPos(sceneLight.orbitData);
        }
    }

    for (const auto& obj : objects) {
        if (obj->orbitData.isOrbiting) {
            const glm::vec3 newPos = CalculateNewPos(obj->orbitData);
            obj->transform[3] = glm::vec4(newPos, 1.0f);
        }
    }

    float sunIntensity = 0.0f;
    if (!m_SceneLights.empty()) {
        // Calculate intensity based on Sun Height (Light 0)
        // If Y > 0 (Day), intensity increases up to 1.0
        float sunHeight = m_SceneLights[0].vulkanLight.position.y;
        sunIntensity = std::clamp(sunHeight / 50.0f, 0.0f, 1.0f);
    }
    UpdateThermodynamics(deltaTime, sunIntensity);

    for (const auto& sys : particleSystems) {
        sys->Update(deltaTime);
    }
}

std::vector<Light> Scene::GetLights() const {
    std::vector<Light> lights;
    lights.reserve(m_SceneLights.size());
    for (const auto& sceneLight : m_SceneLights) {
        lights.push_back(sceneLight.vulkanLight);
    }
    return lights;
}

void Scene::Clear() {
    for (const auto& obj : objects) {
        if (obj && obj->geometry) {
            obj->geometry->Cleanup();
        }
    }
    objects.clear();
    particleSystems.clear();
}

void Scene::SetObjectTransform(size_t index, const glm::mat4& transform) {
    if (index < objects.size()) {
        objects[index]->transform = transform;
    }
}

void Scene::SetObjectLayerMask(const std::string& name, int mask) {
    const auto it = std::find_if(objects.begin(), objects.end(),
        [&name](const std::unique_ptr<SceneObject>& obj) {
            return obj->name == name;
        });
    if (it != objects.end()) {
        (*it)->layerMask = mask;
    }
}

void Scene::SetLightLayerMask(const std::string& name, int mask) {
    const auto it = std::find_if(m_SceneLights.begin(), m_SceneLights.end(),
        [&name](const SceneLight& light) {
            return light.name == name;
        });

    if (it != m_SceneLights.end()) {
        // Update both the SceneLight wrapper and the internal Vulkan struct
        const_cast<SceneLight&>(*it).layerMask = mask;
        const_cast<SceneLight&>(*it).vulkanLight.layerMask = mask;
    }
}

void Scene::SetObjectVisible(size_t index, bool visible) {
    if (index < objects.size()) {
        objects[index]->visible = visible;
    }
}

void Scene::SetObjectCastsShadow(const std::string& name, bool casts) {
    const auto it = std::find_if(objects.begin(), objects.end(),
        [&name](const std::unique_ptr<SceneObject>& obj) {
            return obj->name == name;
        });

    if (it != objects.end()) {
        (*it)->castsShadow = casts;
    }
    else {
        std::cerr << "Warning: Scene object with name '" << name << "' not found to set castsShadow=" << casts << std::endl;
    }
}

void Scene::SetObjectReceivesShadows(const std::string& name, bool receives) {
    const auto it = std::find_if(objects.begin(), objects.end(),
        [&name](const std::unique_ptr<SceneObject>& obj) {
            return obj->name == name;
        });

    if (it != objects.end()) {
        (*it)->receiveShadows = receives;
    }
}

void Scene::SetObjectShadingMode(const std::string& name, int mode) {
    const auto it = std::find_if(objects.begin(), objects.end(),
        [&name](const std::unique_ptr<SceneObject>& obj) {
            return obj->name == name;
        });

    if (it != objects.end()) {
        (*it)->shadingMode = mode;
    }
}

void Scene::UpdateThermodynamics(float deltaTime, float sunIntensity) {
    // 1. Determine Sun Position (Assuming Light 0 is Sun)
    bool globalSunlight = false;
    if (!m_SceneLights.empty()) {
        // Simple check: Is the sun above the horizon?
        globalSunlight = m_SceneLights[0].vulkanLight.position.y > 0.0f;
    }

    for (auto& obj : objects) {
        if (!obj->isFlammable) continue;

        switch (obj->state) {
        case ObjectState::NORMAL:
        case ObjectState::HEATING: {
            // If sun is up and intense enough
            bool inSun = globalSunlight && (sunIntensity > 0.2f);

            if (inSun) {
                obj->state = ObjectState::HEATING;
                obj->currentTemp += obj->heatingRate * sunIntensity * deltaTime;
            }
            else {
                obj->state = ObjectState::NORMAL;
                obj->currentTemp -= obj->coolingRate * deltaTime;
            }

            obj->currentTemp = glm::max(0.0f, obj->currentTemp);

            // --- IGNITION ---
            if (obj->currentTemp >= obj->ignitionThreshold) {
                obj->state = ObjectState::BURNING;
                obj->burnTimer = 0.0f;

                // Spawn Fire (Start small at base)
                glm::vec3 pos = glm::vec3(obj->transform[3]);
                float initialScale = 0.1f;
                obj->fireEmitterId = AddFire(pos, initialScale);

                // Spawn Smoke (Start small, attach to object)
                obj->smokeEmitterId = AddSmoke(pos, initialScale);
            }
            break;
        }

        case ObjectState::BURNING: {
            obj->burnTimer += deltaTime;
            obj->burnFactor = glm::clamp(obj->burnTimer / obj->maxBurnDuration, 0.0f, 1.0f);

            // GROWTH LOGIC: Slower growth over first 60% of burn duration
            float growDuration = obj->maxBurnDuration * 0.6f;
            float growth = glm::clamp(obj->burnTimer / growDuration, 0.0f, 1.0f);

            // Assume object height is roughly 2.0 units (standard tree/bush size in this scene)
            float maxFireHeight = 3.0f;
            float currentFireHeight = 0.2f + (maxFireHeight - 0.2f) * growth;

            glm::vec3 basePos = glm::vec3(obj->transform[3]);

            // Update Fire Emitter (Growing Column)
            if (obj->fireEmitterId != -1) {
                ParticleProps fireProps = ParticleLibrary::GetFireProps();

                // We want the fire to "grow up" from the base.
                fireProps.position = basePos;
                fireProps.position.y += currentFireHeight * 0.5f;

                // Stretch the emitter box vertically
                fireProps.positionVariation.x = 0.3f; // Thin width
                fireProps.positionVariation.z = 0.3f;
                fireProps.positionVariation.y = currentFireHeight * 0.5f; // Extents from center

                // Scale particles slightly
                float particleScale = 1.0f + growth * 0.5f;
                fireProps.sizeBegin *= particleScale;
                fireProps.sizeEnd *= particleScale;

                // Ramp up rate
                float rate = 50.0f + (300.0f * growth);
                GetOrCreateSystem(fireProps)->UpdateEmitter(obj->fireEmitterId, fireProps, rate);
            }

            // Update Smoke Emitter (Follow Top of Fire)
            if (obj->smokeEmitterId != -1) {
                ParticleProps smokeProps = ParticleLibrary::GetSmokeProps();

                // Smoke spawns at the TOP of the fire column
                smokeProps.position = basePos;
                smokeProps.position.y += currentFireHeight;

                // Smoke grows larger as fire gets bigger
                float smokeScale = 1.0f + growth * 2.0f;
                smokeProps.sizeBegin *= smokeScale;
                smokeProps.sizeEnd *= smokeScale;

                // CHANGE: Increased velocity variation (X/Z) so smoke plumes/disperses outward more
                smokeProps.velocityVariation = glm::vec3(2.0f, 1.0f, 2.0f);

                // CHANGE: Increased Lifetime and Vertical Velocity to allow smoke to rise high
                // This prevents the "bunching up" effect
                smokeProps.lifeTime = 8.0f;
                smokeProps.velocity.y = 3.0f;

                float rate = 20.0f + (80.0f * growth);
                GetOrCreateSystem(smokeProps)->UpdateEmitter(obj->smokeEmitterId, smokeProps, rate);
            }

            // --- TURN TO DUST ---
            if (obj->burnTimer >= obj->maxBurnDuration) {
                obj->state = ObjectState::BURNT;

                if (obj->fireEmitterId != -1) {
                    GetOrCreateSystem(ParticleLibrary::GetFireProps())->StopEmitter(obj->fireEmitterId);
                    obj->fireEmitterId = -1;
                }

                // KEEP SMOKE for smoldering (Dust pile)
                if (obj->smokeEmitterId != -1) {
                    ParticleProps dustSmoke = ParticleLibrary::GetSmokeProps();
                    dustSmoke.position = basePos; // Back to ground

                    // Keep smoke very small
                    dustSmoke.sizeBegin *= 0.1f;
                    dustSmoke.sizeEnd *= 0.2f;

                    // CHANGE: Reduced lifetime and vertical velocity so it stays low
                    dustSmoke.lifeTime = 1.0f; // Die quickly
                    dustSmoke.velocity.y = 0.5f; // Rise slowly

                    // Keep constrained position for compactness
                    dustSmoke.positionVariation = glm::vec3(0.1f, 0.0f, 0.1f);
                    dustSmoke.velocityVariation = glm::vec3(0.2f, 0.2f, 0.2f);

                    GetOrCreateSystem(dustSmoke)->UpdateEmitter(obj->smokeEmitterId, dustSmoke, 30.0f);
                }

                obj->regrowTimer = 0.0f;
                obj->burnFactor = 0.0f;

                // 1. Save Original Geometry 
                obj->storedOriginalGeometry = obj->geometry;
                obj->storedOriginalTransform = obj->transform;

                // 2. Assign Dust Geometry 
                if (dustGeometryPrototype) {
                    obj->geometry = dustGeometryPrototype;
                }

                // 3. Swap Texture
                obj->texturePath = sootTexturePath;

                // 4. Shrink visual
                obj->transform = glm::translate(glm::mat4(1.0f), basePos);
                obj->transform = glm::scale(obj->transform, glm::vec3(0.003f)); // Pile of ash size
            }
            break;
        }

        case ObjectState::BURNT: {
            obj->regrowTimer += deltaTime;

            // Stop smoldering after 5 seconds
            if (obj->regrowTimer > 5.0f && obj->smokeEmitterId != -1) {
                GetOrCreateSystem(ParticleLibrary::GetSmokeProps())->StopEmitter(obj->smokeEmitterId);
                obj->smokeEmitterId = -1;
            }

            // --- START REGROWTH ---
            // Condition: Time passed AND it's daytime
            if (obj->regrowTimer >= obj->dustDuration && globalSunlight) {
                obj->state = ObjectState::REGROWING;

                if (obj->smokeEmitterId != -1) {
                    GetOrCreateSystem(ParticleLibrary::GetSmokeProps())->StopEmitter(obj->smokeEmitterId);
                    obj->smokeEmitterId = -1;
                }

                // Restore Geometry Immediately
                if (obj->storedOriginalGeometry) {
                    obj->geometry = obj->storedOriginalGeometry;
                    obj->storedOriginalGeometry = nullptr;
                }

                // Restore Texture
                obj->texturePath = obj->originalTexturePath;

                // Reset Temp
                obj->currentTemp = 0.0f;
            }
            break;
        }

        case ObjectState::REGROWING: {
            obj->regrowTimer += deltaTime;

            const float growthDuration = 2.0f;
            float t = glm::clamp(obj->regrowTimer / growthDuration, 0.0f, 1.0f);

            t = t * t * (3.0f - 2.0f * t);

            float currentScale = glm::mix(0.003f, 1.0f, t);

            obj->transform = glm::scale(obj->storedOriginalTransform, glm::vec3(currentScale));

            if (t >= 1.0f) {
                obj->state = ObjectState::NORMAL;
                obj->burnFactor = 0.0f;
                obj->regrowTimer = 0.0f;
            }
            break;
        }
        }
    }
}
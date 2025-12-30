#include "Scene.h"
#include "ParticleLibrary.h"
#include "../geometry/OBJLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/common.hpp>
#include <iostream>
#include <algorithm>
#include <random>

void Scene::ToggleGlobalShadingMode() {
    globalShadingMode = (globalShadingMode == 1) ? 0 : 1;

    // Update all existing objects, preserving special modes (>=2)
	// 0 = Gouraud, 1 = Phong
    for (auto& obj : objects) {
        if (obj->shadingMode == 0 || obj->shadingMode == 1) {
            obj->shadingMode = globalShadingMode;
        }
    }
    std::cout << "Shading Mode Toggled: " << (globalShadingMode == 1 ? "Phong" : "Gouraud") << std::endl;
}

void Scene::AddObjectInternal(const std::string& name, std::unique_ptr<Geometry> geometry, const glm::vec3& position, const std::string& texturePath, bool isFlammable) {
    std::shared_ptr<Geometry> sharedGeo = std::move(geometry);
    auto obj = std::make_unique<SceneObject>(sharedGeo, texturePath, name);
    obj->transform = glm::translate(glm::mat4(1.0f), position);
    obj->isFlammable = isFlammable;

    // Use the global default instead of poly count check
    obj->shadingMode = globalShadingMode;

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

    // Random distribution for thermal response
    std::uniform_real_distribution<float> distThermal(0.5f, 10.0f);

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

        // 6. Overwrite Transform with Correct Rotation Order & Assign Thermal Stats
        if (!objects.empty()) {
            auto& obj = objects.back();

            // NEW: Assign unique thermal response if the object is flammable
            if (config.isFlammable) {
                obj->thermalResponse = distThermal(gen);
            }

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

    // DISABLE generic cylinder collision for the Terrain object itself 
    // (We will use the Math-based height check instead)
    if (!objects.empty()) {
        objects.back()->hasCollision = false;
    }

    // STORE params for the Camera Controller
    m_TerrainConfig.exists = true;
    m_TerrainConfig.radius = radius;
    m_TerrainConfig.heightScale = heightScale;
    m_TerrainConfig.noiseFreq = noiseFreq;
    m_TerrainConfig.position = position;
}

void Scene::AddBowl(const std::string& name, float radius, int slices, int stacks, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateBowl(device, physicalDevice, radius, slices, stacks), position, texturePath, false);
}

void Scene::AddPedestal(const std::string& name, float topRadius, float baseWidth, float height, const glm::vec3& position, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreatePedestal(device, physicalDevice, topRadius, baseWidth, height, 512, 512), position, texturePath, false);

    if (!objects.empty()) {
        auto& obj = objects.back();
        // Use the wider base for collision to prevent clipping
        obj->collisionRadius = std::max(topRadius, baseWidth);
        obj->collisionHeight = height;
        obj->hasCollision = true;
    }
}

void Scene::AddCube(const std::string& name, const glm::vec3& position, const glm::vec3& scale, const std::string& texturePath) {
    AddObjectInternal(name, GeometryGenerator::CreateCube(device, physicalDevice), position, texturePath, false);

    if (!objects.empty()) {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        t = glm::scale(t, scale);
        objects.back()->transform = t;
        // Logic removed: UpdateShadingMode(objects.back().get());
        // Since AddObjectInternal already sets the global default, we are good.
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

        obj->shadingMode = globalShadingMode; // Use global setting

        objects.push_back(std::move(obj));
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to add model '" << modelPath << "': " << e.what() << std::endl;
    }
}

int Scene::AddLight(const std::string& name, const glm::vec3& position, const glm::vec3& color, float intensity, int type) {
    if (m_SceneLights.size() >= MAX_LIGHTS) {
        std::cerr << "Warning: Maximum number of lights (" << MAX_LIGHTS << ") reached. Light not added." << std::endl;
        return -1;
    }

    SceneLight newSceneLight{};
    newSceneLight.name = name;
    newSceneLight.vulkanLight.position = position;
    newSceneLight.vulkanLight.color = color;
    newSceneLight.vulkanLight.intensity = intensity;
    newSceneLight.vulkanLight.type = type; // We will use Type 1 for Point Lights

    newSceneLight.vulkanLight.layerMask = SceneLayers::INSIDE;
    newSceneLight.layerMask = SceneLayers::INSIDE;

    m_SceneLights.push_back(newSceneLight);

    // Return the index of the newly added light
    return static_cast<int>(m_SceneLights.size() - 1);
}

void Scene::SetObjectCollision(const std::string& name, bool enabled) {
    auto it = std::find_if(objects.begin(), objects.end(), [&](const auto& obj) { return obj->name == name; });
    if (it != objects.end()) {
        (*it)->hasCollision = enabled;
    }
}

void Scene::SetObjectCollisionSize(const std::string& name, float radius, float height) {
    auto it = std::find_if(objects.begin(), objects.end(), [&](const auto& obj) { return obj->name == name; });
    if (it != objects.end()) {
        (*it)->collisionRadius = radius;
        (*it)->collisionHeight = height;
    }
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

void Scene::AddCampfire(const std::string& name, const glm::vec3& position, float scale) {
    // 1. Add Fire Particles
    // (Returns emitter ID, but we ignore it for a static campfire)
    AddFire(position, scale);

    // 2. Add Smoke Particles
    // We offset the smoke slightly upwards so it rises from the top of the flames
    glm::vec3 smokePos = position;
    smokePos.y += 1.5f * scale;
    AddSmoke(smokePos, scale);

    // 3. Add Point Light
    // Position: Center of the flame
    glm::vec3 lightPos = position;
    lightPos.y += 0.5f * scale;

    // Color: Warm Orange (R=1.0, G=0.5, B=0.1)
    glm::vec3 lightColor = glm::vec3(1.0f, 0.5f, 0.1f);

    // Intensity: 
    // We use a base of 2.0, scaled by the fire size. 
    // Since we fixed the attenuation shader, this will look bright but contained.
    float intensity = 1.0f * scale;

    // Type 1 = Point Light (Fire)
    AddLight(name + "_Light", lightPos, lightColor, intensity, 1);
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

void Scene::Ignite(SceneObject* obj) {
    if (!obj || !obj->isFlammable) return;
    if (obj->state == ObjectState::BURNING || obj->state == ObjectState::BURNT) return;

    std::cout << "Igniting object: " << obj->name << std::endl;

    obj->state = ObjectState::BURNING;
    obj->burnTimer = 0.0f;
    obj->currentTemp = obj->ignitionThreshold + 50.0f; // Jumpstart temp to keep it burning

    // Spawn initial particles immediately so we don't wait for the next frame
    glm::vec3 pos = glm::vec3(obj->transform[3]);
    if (obj->fireEmitterId == -1) {
        obj->fireEmitterId = AddFire(pos, 0.1f);
    }
    if (obj->smokeEmitterId == -1) {
        obj->smokeEmitterId = AddSmoke(pos, 0.1f);
    }
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
    // 1. Update Season Cycle
    m_SeasonTimer += deltaTime;
    if (m_SeasonTimer >= m_SeasonDuration) {
        m_SeasonTimer = 0.0f;
        m_CurrentSeason = static_cast<Season>((static_cast<int>(m_CurrentSeason) + 1) % 4);
        std::cout << "Season Changed to: " << GetSeasonName() << std::endl;
    }

    // 2. Calculate Weather
    float sunHeight = 0.0f;
    if (!m_SceneLights.empty()) {
        float rawHeight = m_SceneLights[0].vulkanLight.position.y / 275.0f;
        sunHeight = std::clamp(rawHeight, -1.0f, 1.0f);
    }

    float seasonBaseTemp = 0.0f;
    float dayNightVariation = 0.0f;
    glm::vec3 targetSunColor = glm::vec3(1.0f);

    switch (m_CurrentSeason) {
    case Season::SUMMER:
        // AGGRESSIVE: Base 50C, Day Variation 35C -> Peak Ambient 85C
        seasonBaseTemp = 50.0f;
        dayNightVariation = 35.0f;
        targetSunColor = glm::vec3(1.0f, 0.95f, 0.8f);
        break;
    case Season::AUTUMN:
        seasonBaseTemp = 20.0f;
        dayNightVariation = 15.0f;
        targetSunColor = glm::vec3(1.0f, 0.85f, 0.7f);
        break;
    case Season::WINTER:
        seasonBaseTemp = -5.0f;
        dayNightVariation = 10.0f;
        targetSunColor = glm::vec3(0.75f, 0.85f, 1.0f);
        break;
    case Season::SPRING:
        seasonBaseTemp = 20.0f;
        dayNightVariation = 15.0f;
        targetSunColor = glm::vec3(1.0f, 0.98f, 0.9f);
        break;
    }

    m_WeatherIntensity = seasonBaseTemp + (sunHeight * dayNightVariation);

    // 3. Sun Tint
    auto sunIt = std::find_if(m_SceneLights.begin(), m_SceneLights.end(),
        [](const SceneLight& l) { return l.name == "Sun"; });

    if (sunIt != m_SceneLights.end()) {
        sunIt->vulkanLight.color = glm::mix(sunIt->vulkanLight.color, targetSunColor, deltaTime * 0.8f);
    }

    // 4. Update Orbits
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

    // 5. Update Thermodynamics
    UpdateThermodynamics(deltaTime, sunHeight);

    // 6. Update Particles
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

void Scene::UpdateThermodynamics(float deltaTime, float sunHeight) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);

    // Debug Timer: Prints temp of the first procedural object once per second
    static float printTimer = 0.0f;
    printTimer += deltaTime;
    bool shouldPrint = (printTimer > 1.0f);
    if (shouldPrint) printTimer = 0.0f;

    for (auto& obj : objects) {
        if (!obj->isFlammable) continue;

        // CHANGED: Use the object's unique thermal response instead of a hardcoded value
        float responseSpeed = obj->thermalResponse;

        // 2. Lower threshold to guarantee ignition during "hot" moments
        float effectiveIgnitionThreshold = 100.0f;

        switch (obj->state) {
        case ObjectState::NORMAL:
        case ObjectState::HEATING: {
            float targetTemp = m_WeatherIntensity;

            // 3. Massive Sun Bonus (+60C)
            // If base is 50C + 60C = 110C, well above threshold
            if (sunHeight > 0.1f) {
                float sunIntensity = 60.0f;

                targetTemp += sunIntensity * sunHeight;
            }

            // STABLE MATH: Interpolate towards target (never overshoots)
            float changeRate = responseSpeed * deltaTime;
            float lerpFactor = glm::clamp(changeRate, 0.0f, 1.0f);
            obj->currentTemp = glm::mix(obj->currentTemp, targetTemp, lerpFactor);

            // Debug Print (helps you verify temps are rising)
            if (shouldPrint && obj->name == "ProcObj_0") {
                /* std::cout << "Temp: " << obj->currentTemp
                      << " | Target: " << targetTemp
                      << " | Thresh: " << effectiveIgnitionThreshold << std::endl;*/
            }

            // Visual State Update
            if (obj->currentTemp > 45.0f) {
                obj->state = ObjectState::HEATING;
            }
            else {
                obj->state = ObjectState::NORMAL;
            }

            // IGNITION CHECK
            if (obj->currentTemp >= effectiveIgnitionThreshold) {
                float excessHeat = obj->currentTemp - effectiveIgnitionThreshold;

                // High Chance: % base + 5% per degree of excess heat
                float ignitionChancePerSecond = 0.05f + (excessHeat * 0.005f);

                if (chance(gen) < (ignitionChancePerSecond * deltaTime)) {
                    //std::cout << "IGNITION: " << obj->name << std::endl;
                    obj->state = ObjectState::BURNING;
                    obj->burnTimer = 0.0f;

                    // Spawn Initial Particles
                    glm::vec3 pos = glm::vec3(obj->transform[3]);
                    obj->fireEmitterId = AddFire(pos, 0.1f);
                    obj->smokeEmitterId = AddSmoke(pos, 0.1f);
                }
            }
            break;
        }

        case ObjectState::BURNING: {
            // Self-Heating: Fire generates its own heat
            obj->currentTemp += obj->selfHeatingRate * deltaTime;
            obj->burnTimer += deltaTime;

            // Calculate Growth Factors
            float growth = glm::clamp(obj->burnTimer / (obj->maxBurnDuration * 0.6f), 0.0f, 1.0f);
            obj->burnFactor = glm::clamp(obj->burnTimer / obj->maxBurnDuration, 0.0f, 1.0f);

            // Fire Height Calculation
            float maxFireHeight = 3.0f;
            float currentFireHeight = 0.2f + (maxFireHeight - 0.2f) * growth;
            glm::vec3 basePos = glm::vec3(obj->transform[3]);

            // Update Fire Particles
            if (obj->fireEmitterId != -1) {
                ParticleProps fireProps = ParticleLibrary::GetFireProps();
                fireProps.position = basePos;
                fireProps.position.y += currentFireHeight * 0.5f;
                fireProps.positionVariation = glm::vec3(0.3f, currentFireHeight * 0.4f, 0.3f);

                float particleScale = 1.0f + growth * 0.5f;
                fireProps.sizeBegin *= particleScale;
                fireProps.sizeEnd *= particleScale;

                float rate = 50.0f + (300.0f * growth);
                GetOrCreateSystem(fireProps)->UpdateEmitter(obj->fireEmitterId, fireProps, rate);
            }

            // Update Smoke Particles
            if (obj->smokeEmitterId != -1) {
                ParticleProps smokeProps = ParticleLibrary::GetSmokeProps();
                smokeProps.position = basePos;
                smokeProps.position.y += currentFireHeight;

                float smokeScale = 1.0f + growth * 2.0f;
                smokeProps.sizeBegin *= smokeScale;
                smokeProps.sizeEnd *= smokeScale;
                smokeProps.lifeTime = 8.0f;
                smokeProps.velocity.y = 3.0f;

                float rate = 20.0f + (80.0f * growth);
                GetOrCreateSystem(smokeProps)->UpdateEmitter(obj->smokeEmitterId, smokeProps, rate);
            }

            // Update Fire Light
            glm::vec3 lightPos = basePos;
            lightPos.y += currentFireHeight * 0.5f;

            if (obj->fireLightIndex == -1) {
                obj->fireLightIndex = AddLight(obj->name + "_Fire", lightPos, glm::vec3(1.0f, 0.5f, 0.1f), 0.0f, 1);
            }

            if (obj->fireLightIndex != -1 && obj->fireLightIndex < m_SceneLights.size()) {
                float t = obj->burnTimer;
                float flicker = 1.0f + 0.3f * std::sin(t * 15.0f) + 0.15f * std::sin(t * 37.0f);

                float targetIntensity = 50.05f * growth; // flame_intensity

                m_SceneLights[obj->fireLightIndex].vulkanLight.position = lightPos;
                m_SceneLights[obj->fireLightIndex].vulkanLight.intensity = targetIntensity * flicker;
            }

            // Transition to Burnt (Ash)
            if (obj->burnTimer >= obj->maxBurnDuration) {
                obj->state = ObjectState::BURNT;

                // Stop Fire
                if (obj->fireEmitterId != -1) {
                    GetOrCreateSystem(ParticleLibrary::GetFireProps())->StopEmitter(obj->fireEmitterId);
                    obj->fireEmitterId = -1;
                }
                // Stop Light
                if (obj->fireLightIndex != -1 && obj->fireLightIndex < m_SceneLights.size()) {
                    m_SceneLights[obj->fireLightIndex].vulkanLight.intensity = 0.0f;
                }
                // Switch Smoke to Smoldering
                if (obj->smokeEmitterId != -1) {
                    ParticleProps smolder = ParticleLibrary::GetSmokeProps();
                    smolder.position = basePos;
                    smolder.sizeBegin *= 0.1f;
                    smolder.sizeEnd *= 0.2f;
                    smolder.lifeTime = 1.5f;
                    smolder.velocity.y = 0.5f;
                    smolder.positionVariation = glm::vec3(0.1f);
                    GetOrCreateSystem(smolder)->UpdateEmitter(obj->smokeEmitterId, smolder, 20.0f);
                }

                // Swap Geometry
                obj->storedOriginalGeometry = obj->geometry;
                obj->storedOriginalTransform = obj->transform;

                if (dustGeometryPrototype) {
                    obj->geometry = dustGeometryPrototype;
                }
                obj->texturePath = sootTexturePath;

                // Shrink
                obj->transform = glm::translate(glm::mat4(1.0f), basePos);
                obj->transform = glm::scale(obj->transform, glm::vec3(0.003f));

                obj->regrowTimer = 0.0f;
                obj->burnFactor = 0.0f;
            }
            break;
        }

        case ObjectState::BURNT:
        case ObjectState::REGROWING: {
            // Stable Cooling towards ambient
            float changeRate = 0.5f * deltaTime;
            float lerpFactor = glm::clamp(changeRate, 0.0f, 1.0f);
            obj->currentTemp = glm::mix(obj->currentTemp, m_WeatherIntensity, lerpFactor);

            obj->regrowTimer += deltaTime;

            // Stop smoldering after 5s
            if (obj->state == ObjectState::BURNT && obj->regrowTimer > 5.0f && obj->smokeEmitterId != -1) {
                GetOrCreateSystem(ParticleLibrary::GetSmokeProps())->StopEmitter(obj->smokeEmitterId);
                obj->smokeEmitterId = -1;
            }

            if (obj->state == ObjectState::BURNT) {
                // Only regrow if weather is warm (> 10C)
                bool isWarmWeather = m_WeatherIntensity > 10.0f;

                if (obj->regrowTimer >= obj->dustDuration && isWarmWeather) {
                    obj->state = ObjectState::REGROWING;
                    obj->regrowTimer = 0.0f;

                    // Reset temp immediately so it doesn't loop back to burning
                    obj->currentTemp = m_WeatherIntensity;

                    if (obj->storedOriginalGeometry) {
                        obj->geometry = obj->storedOriginalGeometry;
                        obj->storedOriginalGeometry = nullptr;
                    }
                    obj->texturePath = obj->originalTexturePath;
                }
            }
            else if (obj->state == ObjectState::REGROWING) {
                // Pop-up animation
                const float growthTime = 2.0f;
                float t = glm::clamp(obj->regrowTimer / growthTime, 0.0f, 1.0f);
                t = t * t * (3.0f - 2.0f * t); // Smoothstep

                float currentScale = glm::mix(0.003f, 1.0f, t);
                obj->transform = glm::scale(obj->storedOriginalTransform, glm::vec3(currentScale));

                if (t >= 1.0f) {
                    obj->state = ObjectState::NORMAL;
                    obj->currentTemp = m_WeatherIntensity;
                }
            }
            break;
        }
        }
    }
}

std::string Scene::GetSeasonName() const {
    switch (m_CurrentSeason) {
    case Season::SUMMER: return "Summer";
    case Season::AUTUMN: return "Autumn";
    case Season::WINTER: return "Winter";
    case Season::SPRING: return "Spring";
    }
    return "Unknown";
}

void Scene::ResetEnvironment() {
    // 1. Reset Lights (Sun/Moon orbits)
    for (auto& light : m_SceneLights) {
        if (light.orbitData.isOrbiting) {
            light.orbitData.currentAngle = light.orbitData.initialAngle;
            // Recalculate position immediately
            glm::quat rotation = glm::angleAxis(light.orbitData.currentAngle, light.orbitData.axis);
            glm::vec3 offset = rotation * glm::vec3(light.orbitData.radius, 0.0f, 0.0f);
            light.vulkanLight.position = light.orbitData.center + offset;
        }
    }

    // 2. Reset Objects
    for (auto& obj : objects) {
        // A. Reset Orbit
        if (obj->orbitData.isOrbiting) {
            obj->orbitData.currentAngle = obj->orbitData.initialAngle;
            glm::quat rotation = glm::angleAxis(obj->orbitData.currentAngle, obj->orbitData.axis);
            glm::vec3 offset = rotation * glm::vec3(obj->orbitData.radius, 0.0f, 0.0f);
            obj->transform[3] = glm::vec4(obj->orbitData.center + offset, 1.0f);
        }

        // B. Reset Thermodynamics / Visual State
        if (obj->isFlammable) {
            // Stop any active particles
            if (obj->fireEmitterId != -1) {
                GetOrCreateSystem(ParticleLibrary::GetFireProps())->StopEmitter(obj->fireEmitterId);
                obj->fireEmitterId = -1;
            }
            if (obj->smokeEmitterId != -1) {
                GetOrCreateSystem(ParticleLibrary::GetSmokeProps())->StopEmitter(obj->smokeEmitterId);
                obj->smokeEmitterId = -1;
            }

            // Restore Geometry if it was swapped (Burnt state)
            if (obj->storedOriginalGeometry) {
                obj->geometry = obj->storedOriginalGeometry;
                obj->storedOriginalGeometry = nullptr;
                obj->transform = obj->storedOriginalTransform; // Restore original scale/transform
            }
            else if (obj->state == ObjectState::REGROWING) {
                // If it was regrowing, it might be using original geometry but scaled down
                obj->transform = obj->storedOriginalTransform;
            }

            // Restore Texture
            obj->texturePath = obj->originalTexturePath;

            // Reset Variables
            obj->state = ObjectState::NORMAL;
            obj->currentTemp = 0.0f;
            obj->burnTimer = 0.0f;
            obj->regrowTimer = 0.0f;
            obj->burnFactor = 0.0f;
        }
    }
}
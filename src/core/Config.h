#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct ProceduralPlantConfig {
    std::string modelPath;
    std::string texturePath;
    float frequency;
    glm::vec3 minScale;
    glm::vec3 maxScale;
    glm::vec3 baseRotation; // NEW: Added this
    bool isFlammable;
};

struct StaticObjectConfig {
    std::string name;
    std::string modelPath;
    std::string texturePath;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    bool isFlammable;
};

struct SeasonConfig {
    float durationSeconds = 60.0f;
    float summerBaseTemp = 50.0f;
    float winterBaseTemp = -5.0f;
    float dayNightTempDiff = 35.0f;
};

struct CACTIConfig {
    glm::vec3 axis = glm::vec3(0.0f, 0.0f, 1.0f);
    float speed = 0.1f;
    float radius = 275.0f;
    float initialAngle = 0.0f;
};

struct AppConfig {
    // Window
    int windowWidth = 800;
    int windowHeight = 600;

    // Environment
    SeasonConfig seasons;
    CACTIConfig sunCACTI;
    CACTIConfig moonCACTI;

    // Thermodynamics
    float sunHeatBonus = 60.0f;

    // Terrain
    float terrainHeightScale = 3.5f;
    float terrainNoiseFreq = 0.02f;

    // Objects
    std::vector<ProceduralPlantConfig> proceduralPlants;
    std::vector<StaticObjectConfig> staticObjects;
};

class ConfigLoader {
public:
    static AppConfig Load(const std::string& filepath);
};
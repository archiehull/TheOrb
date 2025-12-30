#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>

AppConfig ConfigLoader::Load(const std::string& filepath) {
    AppConfig config;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Could not open config file: " << filepath << ". Using defaults." << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string key;
        ss >> key;

        if (key == "WindowSize") {
            ss >> config.windowWidth >> config.windowHeight;
        }
        else if (key == "SeasonDuration") {
            ss >> config.seasons.durationSeconds;
        }
        else if (key == "SeasonTemps") {
            ss >> config.seasons.summerBaseTemp >> config.seasons.winterBaseTemp >> config.seasons.dayNightTempDiff;
        }
        else if (key == "SunOrbit") {
            ss >> config.sunOrbit.axis.x >> config.sunOrbit.axis.y >> config.sunOrbit.axis.z
                >> config.sunOrbit.radius >> config.sunOrbit.speed >> config.sunOrbit.initialAngle;
        }
        else if (key == "MoonOrbit") {
            ss >> config.moonOrbit.axis.x >> config.moonOrbit.axis.y >> config.moonOrbit.axis.z
                >> config.moonOrbit.radius >> config.moonOrbit.speed >> config.moonOrbit.initialAngle;
        }
        else if (key == "SunHeatBonus") {
            ss >> config.sunHeatBonus;
        }
        else if (key == "TerrainParams") {
            ss >> config.terrainHeightScale >> config.terrainNoiseFreq;
        }
        else if (key == "ProceduralPlant") {
            ProceduralPlantConfig plant;
            std::string flammableStr;
            // Format: Model Tex Freq MinScale(3) MaxScale(3) BaseRot(3) Flammable
            ss >> plant.modelPath >> plant.texturePath >> plant.frequency
                >> plant.minScale.x >> plant.minScale.y >> plant.minScale.z
                >> plant.maxScale.x >> plant.maxScale.y >> plant.maxScale.z
                >> plant.baseRotation.x >> plant.baseRotation.y >> plant.baseRotation.z
                >> flammableStr;
            plant.isFlammable = (flammableStr == "1" || flammableStr == "true");
            config.proceduralPlants.push_back(plant);
        }
        else if (key == "StaticObject") {
            StaticObjectConfig obj;
            std::string flammableStr;
            // Format: Name Model Tex Pos(3) Rot(3) Scale(3) Flammable
            ss >> obj.name >> obj.modelPath >> obj.texturePath
                >> obj.position.x >> obj.position.y >> obj.position.z
                >> obj.rotation.x >> obj.rotation.y >> obj.rotation.z
                >> obj.scale.x >> obj.scale.y >> obj.scale.z
                >> flammableStr;
            obj.isFlammable = (flammableStr == "1" || flammableStr == "true");
            config.staticObjects.push_back(obj);
        }
    }

    return config;
}
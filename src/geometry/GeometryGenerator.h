//
#pragma once

#include "Geometry.h"
#include <glm/glm.hpp>

class GeometryGenerator {
public:
    static std::unique_ptr<Geometry> CreateCube(VkDevice device, VkPhysicalDevice physicalDevice);
    static std::unique_ptr<Geometry> CreateGrid(VkDevice device, VkPhysicalDevice physicalDevice,
        int rows, int cols, float cellSize = 0.1f);
    static std::unique_ptr<Geometry> CreateSphere(VkDevice device, VkPhysicalDevice physicalDevice,
        int stacks = 16, int slices = 32, float radius = 0.5f);

    static std::unique_ptr<Geometry> CreateTerrain(VkDevice device, VkPhysicalDevice physicalDevice,
        float radius, int rings, int segments, float heightScale, float noiseFreq);

private:
    static glm::vec3 GenerateColor(int index, int total);
};
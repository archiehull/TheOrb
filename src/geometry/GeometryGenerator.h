#pragma once

#include "Geometry.h"
#include <glm/glm.hpp>

class GeometryGenerator {
public:
    static std::unique_ptr<Geometry> CreateTriangle(VkDevice device, VkPhysicalDevice physicalDevice);
    static std::unique_ptr<Geometry> CreateQuad(VkDevice device, VkPhysicalDevice physicalDevice);
    static std::unique_ptr<Geometry> CreateCircle(VkDevice device, VkPhysicalDevice physicalDevice,
        int segments = 32, float radius = 0.5f);
    static std::unique_ptr<Geometry> CreateCube(VkDevice device, VkPhysicalDevice physicalDevice);
    static std::unique_ptr<Geometry> CreateGrid(VkDevice device, VkPhysicalDevice physicalDevice,
        int rows, int cols, float cellSize = 0.1f);

private:
    static glm::vec3 GenerateColor(int index, int total);
};
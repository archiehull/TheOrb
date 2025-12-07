#pragma once

#include <glm/glm.hpp>

// Simple UBO for basic 3D rendering
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

// You can add more UBO types as needed:
// struct LightingUBO {
//     alignas(16) glm::vec3 lightPos;
//     alignas(16) glm::vec3 lightColor;
//     alignas(16) glm::vec3 viewPos;
// };
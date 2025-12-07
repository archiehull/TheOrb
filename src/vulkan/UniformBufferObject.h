#pragma once

#include <glm/glm.hpp>

// Uniform buffer for shared data (view and projection)
struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};
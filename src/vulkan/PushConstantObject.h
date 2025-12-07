#pragma once

#include <glm/glm.hpp>

// Push constant for per-object data (model matrix)
struct PushConstantObject {
    alignas(16) glm::mat4 model;
};
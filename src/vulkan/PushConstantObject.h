#pragma once
#include <glm/glm.hpp>

struct PushConstantObject {
    alignas(16) glm::mat4 model;
    alignas(4) int shadingMode; // 0 = Gouraud, 1 = Phong
    alignas(4) int receiveShadows;
    alignas(4) int layerMask;
};
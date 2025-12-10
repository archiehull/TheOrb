#pragma once
#include <glm/glm.hpp>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define MAX_LIGHTS 32

struct Light
{
    alignas(16) glm::vec3 position; // offset 0
    alignas(16) glm::vec3 color;    // offset 16
    float intensity;                // offset 32
    int type;                       // offset 36
    float padding[2];               // offset 40, size 8 -> total size 48
};

struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPos;
    
    // Explicit padding to ensure 'lights' starts at 16-byte aligned offset if needed
    // (vec3 is 12 bytes, alignment 16. struct size so far is 128 + 12 = 140. 
    // Next array must start at 144. Compiler usually handles this padding, but be careful.)
    
    alignas(16) Light lights[MAX_LIGHTS];
    alignas(4) int numLights;
};
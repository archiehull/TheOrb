#version 450

// Uniform buffer for view and projection (shared across all objects)
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

// Push constant for model matrix (per-object)
layout(push_constant) uniform PushConstantObject {
    mat4 model;
} pco;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord; // added

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV; // added

void main() {
    gl_Position = ubo.proj * ubo.view * pco.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragUV = inTexCoord;
}
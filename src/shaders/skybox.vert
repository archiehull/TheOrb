#version 450
layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstantObject {
    mat4 model;
    int shadingMode;
} pco;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    mat4 lightSpaceMatrix;
} ubo;

layout(location = 0) out vec3 outUVW;

void main() {
    // Use the local model position for texture sampling.
    // This anchors the skybox to the sphere, creating a stable "Crystal Ball" effect.
    outUVW = inPosition;
    
    // Standard projection for a physical object in the room
    gl_Position = ubo.proj * ubo.view * pco.model * vec4(inPosition, 1.0);
}
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

// Set 1: Per-Object Texture
layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragUV);
}
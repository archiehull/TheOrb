#version 450

#define MAX_LIGHTS 32

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    int type;
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec3 fragGouraudColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    Light lights[MAX_LIGHTS]; // Updated to Array
    int numLights;
} ubo;

layout(push_constant) uniform PushConstantObject {
    mat4 model;
    int shadingMode;
} pco;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragUV);
    vec3 lighting = vec3(0.0);

    if (pco.shadingMode == 0) {
        // Gouraud (calculated in vertex)
        lighting = fragGouraudColor;
    } else {
        // Phong (calculated here per pixel)
        vec3 normal = normalize(fragNormal);
        vec3 viewDir = normalize(ubo.viewPos - fragPos);

        for(int i = 0; i < ubo.numLights; i++) {
            // Ambient
            float ambientStrength = 0.1;
            vec3 ambient = ambientStrength * ubo.lights[i].color * ubo.lights[i].intensity;

            // Diffuse
            vec3 lightDir = normalize(ubo.lights[i].position - fragPos);
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * ubo.lights[i].color * ubo.lights[i].intensity;

            // Specular
            float specularStrength = 0.5;
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * ubo.lights[i].color * ubo.lights[i].intensity;

            lighting += (ambient + diffuse + specular);
        }
    }

    outColor = vec4(lighting * texColor.rgb, texColor.a);
}
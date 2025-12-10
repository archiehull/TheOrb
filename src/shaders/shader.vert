#version 450

#define MAX_LIGHTS 32

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    int type;
};

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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out vec3 fragGouraudColor;

void main() {
    vec4 worldPos = pco.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    fragColor = inColor;
    fragUV = inTexCoord;
    
    mat3 normalMatrix = mat3(transpose(inverse(pco.model)));
    vec3 normal = normalize(normalMatrix * inNormal);
    
    fragNormal = normal;
    fragPos = vec3(worldPos);

    // GOURAUD SHADING (Loop through lights)
    if (pco.shadingMode == 0) {
        vec3 finalColor = vec3(0.0);
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

            finalColor += (ambient + diffuse + specular);
        }
        fragGouraudColor = finalColor;
    } else {
        fragGouraudColor = vec3(0.0);
    }
}
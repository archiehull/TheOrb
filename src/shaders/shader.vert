#version 450

#define MAX_LIGHTS 32

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    int type;
    int layerMask;  
    float padding;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    mat4 lightSpaceMatrix;
    Light lights[MAX_LIGHTS];
    int numLights;
} ubo;

layout(push_constant) uniform PushConstantObject {
    mat4 model;
    int shadingMode;
    int receiveShadows; 
    int layerMask;      
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
layout(location = 5) out vec4 fragPosLightSpace;
layout(location = 6) out vec3 fragOtherLightColor;

void main() {
    vec4 worldPos = pco.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    fragColor = inColor;
    fragUV = inTexCoord;
    
    mat3 normalMatrix = mat3(transpose(inverse(pco.model)));
    vec3 normal = normalize(normalMatrix * inNormal);
    
    fragNormal = normal;
    fragPos = vec3(worldPos);

    fragPosLightSpace = ubo.lightSpaceMatrix * worldPos;

    // Initialize accumulators
    vec3 sunColor = vec3(0.0);
    vec3 otherColor = vec3(0.0);

    // GOURAUD SHADING CALCULATION
    if (pco.shadingMode == 0) {
        vec3 viewDir = normalize(ubo.viewPos - fragPos);

        for(int i = 0; i < ubo.numLights; i++) {
            if ((ubo.lights[i].layerMask & pco.layerMask) == 0) {
                continue;
            }

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

            vec3 result = ambient + diffuse + specular;

            // Separate Sun (0) from Moon (1+)
            if (i == 0) {
                sunColor += result;
            } else {
                otherColor += result;
            }
        }
    }
    
    fragGouraudColor = sunColor;
    fragOtherLightColor = otherColor;
}
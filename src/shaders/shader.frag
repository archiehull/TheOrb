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

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec3 fragGouraudColor;
layout(location = 5) in vec4 fragPosLightSpace;
layout(location = 6) in vec3 fragOtherLightColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    mat4 lightSpaceMatrix;
    Light lights[MAX_LIGHTS];
    int numLights;
    float dayNightFactor; // Ensure this matches C++ UBO
} ubo;

layout(push_constant) uniform PushConstantObject {
    mat4 model;
    int shadingMode;
    int receiveShadows;
    int layerMask;
} pco;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;
layout(set = 0, binding = 2) uniform sampler2D refractionSampler; 

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(location = 0) out vec4 outColor;

float ShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    float currentDepth = projCoords.z;

    if(projCoords.z > 1.0 || projCoords.z < 0.0) return 0.0;
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(ubo.lights[0].position - fragPos);
    
    // Bias: Prevent shadow acne
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.0001); 

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

void main() {
    // --- 1. SCREEN-SPACE REFRACTION MODE (Mode 3: Inner Sphere) ---
    if (pco.shadingMode == 3) {
        vec3 I = normalize(fragPos - ubo.viewPos);
        vec3 N = normalize(fragNormal);
        
        vec2 viewportSize = vec2(textureSize(refractionSampler, 0));
        vec2 screenUV = gl_FragCoord.xy / viewportSize;

        vec2 distortion = N.xy * 0.40;
        vec2 refractedUV = screenUV + distortion;

        vec3 refractionColor = texture(refractionSampler, refractedUV).rgb;
        vec3 foggyTint = vec3(0.9, 0.95, 1.0) * 0.2;
        vec3 baseColor = mix(refractionColor, foggyTint, 0.6);

        vec3 lightDir = normalize(ubo.lights[0].position - fragPos);
        vec3 H = normalize(lightDir - I);
        float spec = pow(max(dot(N, H), 0.0), 64.0);
        vec3 specularColor = spec * ubo.lights[0].color * 1.5;

        float fresnel = pow(1.0 - max(dot(-I, N), 0.0), 2.0);
        float alpha = clamp(0.2 + (fresnel * 0.7), 0.0, 1.0);
        
        outColor = vec4(baseColor + specularColor, alpha);
        return;
    }

    // --- 2. DARK GLOSS SHELL (Mode 4: Outer Sphere) ---
    if (pco.shadingMode == 4) {
        vec3 I = normalize(fragPos - ubo.viewPos);
        vec3 N = normalize(fragNormal);

        vec3 darkTint = vec3(0.02, 0.02, 0.05);
        vec3 lightDir = normalize(ubo.lights[0].position - fragPos);
        vec3 H = normalize(lightDir - I);

        float spec = pow(max(dot(N, H), 0.0), 8.0);
        vec3 specularColor = spec * ubo.lights[0].color * 0.5;
        float fresnel = pow(1.0 - max(dot(-I, N), 0.0), 3.0);
        float alpha = clamp(0.25 + (fresnel * 0.6), 0.0, 1.0);

        outColor = vec4(darkTint + specularColor, alpha);
        return;
    }

    // --- STANDARD LIGHTING (Modes 0 & 1) ---
    vec4 texColor = texture(texSampler, fragUV);
    vec3 lighting = vec3(0.0);

    float shadow = ShadowCalculation(fragPosLightSpace);

    // --- NEW LOGIC: Fade shadows at low sun angles ---
    // Start fading when Sun Y < 30.0, fully gone by Y = -10.0
    if (ubo.numLights > 0) {
        float sunHeight = ubo.lights[0].position.y;
        float fadeStart = 100.0;
        float fadeEnd = 15.0; // Stop shadows while sun is still somewhat up
        
        float shadowFade = clamp((sunHeight - fadeEnd) / (fadeStart - fadeEnd), 0.0, 1.0);
        
        // Smooth transition
        shadowFade = shadowFade * shadowFade * (3.0 - 2.0 * shadowFade);
        
        shadow *= shadowFade;
    }

    if (pco.receiveShadows == 0) {
        shadow = 0.0;
    }

    if (pco.shadingMode == 0) {
        // Gouraud
        lighting = fragGouraudColor * (1.0 - shadow) + fragOtherLightColor;
    } else {
        // Phong
        vec3 normal = normalize(fragNormal);
        vec3 viewDir = normalize(ubo.viewPos - fragPos);

        for(int i = 0; i < ubo.numLights; i++) {
            if ((ubo.lights[i].layerMask & pco.layerMask) == 0) {
                continue;
            }   

            float ambientStrength = 0.1;
            vec3 ambient = ambientStrength * ubo.lights[i].color * ubo.lights[i].intensity;

            vec3 lightDir = normalize(ubo.lights[i].position - fragPos);
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * ubo.lights[i].color * ubo.lights[i].intensity;

            float specularStrength = 0.5;
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * ubo.lights[i].color * ubo.lights[i].intensity;
            
            float lightShadow = 0.0;
            // Only Light 0 (Sun) casts shadows in this setup
            if (i == 0) {
                lightShadow = shadow;
            }

            lighting += (ambient + (1.0 - lightShadow) * (diffuse + specular));
        }
    }

    outColor = vec4(lighting * texColor.rgb, texColor.a);
}
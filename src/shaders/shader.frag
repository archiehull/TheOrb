#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec3 fragGouraudColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
} ubo;

layout(push_constant) uniform PushConstantObject {
    mat4 model;
    int shadingMode; // 0 = Gouraud, 1 = Phong
} pco;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragUV);
    vec3 lighting;

    if (pco.shadingMode == 0) {
        // GOURAUD: Use calculated color from vertex shader
        lighting = fragGouraudColor;
    } else {
        // PHONG SHADING (Per-Fragment)
        vec3 normal = normalize(fragNormal);
        
        // Ambient
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * ubo.lightColor;
        
        // Diffuse
        vec3 lightDir = normalize(ubo.lightPos - fragPos);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * ubo.lightColor;
        
        // Specular
        float specularStrength = 0.5;
        vec3 viewDir = normalize(ubo.viewPos - fragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * ubo.lightColor;
        
        lighting = (ambient + diffuse + specular);
    }

    outColor = vec4(lighting * texColor.rgb, texColor.a);
}
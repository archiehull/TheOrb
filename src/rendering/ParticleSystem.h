#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "GraphicsPipeline.h"
#include "Texture.h"
#include "../vulkan/VulkanBuffer.h"

struct ParticleProps {
    glm::vec3 position;
    glm::vec3 velocity, velocityVariation;
    glm::vec4 colorBegin, colorEnd;
    float sizeBegin, sizeEnd, sizeVariation;
    float lifeTime;
};

class ParticleSystem {
public:
    ParticleSystem(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, uint32_t maxParticles);
    ~ParticleSystem();

    void Initialize(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout, const std::string& texturePath, bool additiveBlending);
    void Update(float dt);
    void Draw(VkCommandBuffer cmd, VkDescriptorSet globalDescriptorSet);

    void Emit(const ParticleProps& props);
    void AddEmitter(const ParticleProps& props, float particlesPerSecond);

private:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 colorBegin, colorEnd;
        float sizeBegin, sizeEnd;
        float lifeTime, lifeRemaining;
        bool active = false;
        float camDistance = -1.0f; // For sorting
    };

    // Data sent to GPU per instance
    struct InstanceData {
        glm::vec3 position;
        glm::vec4 color;
        float size;
    };

    struct ParticleEmitter {
        ParticleProps props;
        float particlesPerSecond;
        float timeSinceLastEmit;
    };

    std::vector<ParticleEmitter> emitters;

    VkDevice device;
    VkPhysicalDevice physicalDevice;

    std::vector<Particle> particles;
    uint32_t maxParticles;
    uint32_t poolIndex = 9999; // Simple circular buffer index

    // Rendering resources
    std::unique_ptr<GraphicsPipeline> pipeline;
    std::unique_ptr<Texture> texture;
    std::unique_ptr<VulkanBuffer> vertexBuffer;   // The Quad (4 verts)
    std::unique_ptr<VulkanBuffer> instanceBuffer; // The Instance Data

    VkDescriptorSet descriptorSet; // For Texture
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout textureLayout;

    void SetupPipeline(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout, bool additive);
    void SetupBuffers();
    void UpdateInstanceBuffer();
};
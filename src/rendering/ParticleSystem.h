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

    // Modified: Accepts shared pipeline resources
    void Initialize(VkDescriptorSetLayout textureLayout, GraphicsPipeline* pipeline, const std::string& texturePath);

    void Update(float dt);
    void Draw(VkCommandBuffer cmd, VkDescriptorSet globalDescriptorSet);

    void Emit(const ParticleProps& props);
    void AddEmitter(const ParticleProps& props, float particlesPerSecond);

    // Data sent to GPU per instance (Made public for pipeline config)
    struct InstanceData {
        glm::vec3 position;
        glm::vec4 color;
        float size;
    };

    // Static helpers to describe vertex input for the shared pipeline
    static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

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
    uint32_t poolIndex = 9999;

    // Rendering resources
    GraphicsPipeline* pipeline = nullptr; // Weak pointer to shared pipeline

    std::unique_ptr<Texture> texture;
    std::unique_ptr<VulkanBuffer> vertexBuffer;
    std::unique_ptr<VulkanBuffer> instanceBuffer;

    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool;

    // We do not own the layout anymore
    VkDescriptorSetLayout textureLayout = VK_NULL_HANDLE;

    void SetupBuffers();
    void UpdateInstanceBuffer();
};
#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "GraphicsPipeline.h"
#include "Texture.h"
#include "../vulkan/VulkanBuffer.h"

struct ParticleProps {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 velocityVariation = glm::vec3(0.0f);
    glm::vec4 colorBegin = glm::vec4(1.0f);
    glm::vec4 colorEnd = glm::vec4(1.0f);
    float sizeBegin = 1.0f, sizeEnd = 1.0f, sizeVariation = 0.0f; // Default to 0 safety
    float lifeTime = 1.0f;
};

class ParticleSystem {
public:
    ParticleSystem(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, uint32_t maxParticles, uint32_t framesInFlight);
    ~ParticleSystem();

    // Modified: Accepts shared pipeline resources
    void Initialize(VkDescriptorSetLayout textureLayout, GraphicsPipeline* pipeline, const std::string& texturePath);

    void Update(float dt);
    void Draw(VkCommandBuffer cmd, VkDescriptorSet globalDescriptorSet, uint32_t currentFrame);

    void Emit(const ParticleProps& props);
    void AddEmitter(const ParticleProps& props, float particlesPerSecond);


    // Data sent to GPU per instance (Modified for 16-byte alignment)
    struct InstanceData {
        glm::vec4 position; // xyz = position, w = padding (Offset 0)
        glm::vec4 color;    // rgba (Offset 16)
        glm::vec4 size;     // x = size, yzw = padding   (Offset 32)
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
    uint32_t framesInFlight;
    uint32_t poolIndex = 9999;

    // Rendering resources
    GraphicsPipeline* pipeline = nullptr; // Weak pointer to shared pipeline

    std::unique_ptr<Texture> texture;
    std::unique_ptr<VulkanBuffer> vertexBuffer;
    std::vector<std::unique_ptr<VulkanBuffer>> instanceBuffers;

    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool;

    // We do not own the layout anymore
    VkDescriptorSetLayout textureLayout = VK_NULL_HANDLE;

    void SetupBuffers();
    void UpdateInstanceBuffer(uint32_t currentFrame);
};
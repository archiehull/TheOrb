#pragma once

#include "SkyboxPass.h"
#include "Cubemap.h"
#include "Scene.h"
#include "../vulkan/VulkanDevice.h"
#include "../vulkan/VulkanSwapChain.h"
#include "../vulkan/VulkanRenderPass.h"
#include "../vulkan/VulkanCommandBuffer.h"
#include "../vulkan/VulkanSyncObjects.h"
#include "../vulkan/VulkanDescriptorSet.h"
#include "../vulkan/UniformBufferObject.h"
#include "../vulkan/PushConstantObject.h"
#include "../rendering/GraphicsPipeline.h"
#include "../rendering/Texture.h"
#include "../rendering/ShadowPass.h"

#include <memory>
#include <map>
#include <vulkan/VulkanContext.h>
#include "Camera.h"

class Renderer {
public:
    Renderer(VulkanDevice* device, VulkanSwapChain* swapChain);
    ~Renderer();

    void Initialize();
    void DrawFrame(Scene& scene, uint32_t currentFrame, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    void RecreateSwapChain();
    void UpdateUniformBuffer(uint32_t currentFrame, const UniformBufferObject& ubo);
    void WaitIdle();
    void Cleanup();

    VulkanRenderPass* GetRenderPass() { return renderPass.get(); }
    GraphicsPipeline* GetPipeline() { return graphicsPipeline.get(); }

private:
    // Initialized to nullptr to satisfy static analysis and avoid use-before-init
    Camera* m_camera = nullptr;
    VulkanContext* m_vulkanContext = nullptr;
    std::vector<void*> m_uniformBuffersMapped;

    VulkanDevice* device;
    VulkanSwapChain* swapChain;

    std::unique_ptr<VulkanRenderPass> renderPass;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;
    std::unique_ptr<VulkanCommandBuffer> commandBuffer;
    std::unique_ptr<VulkanSyncObjects> syncObjects;
    std::unique_ptr<ShadowPass> shadowPass;
    std::unique_ptr<SkyboxPass> skyboxPass;

    VkImage offScreenImage = VK_NULL_HANDLE;
    VkDeviceMemory offScreenImageMemory = VK_NULL_HANDLE;
    VkImageView offScreenImageView = VK_NULL_HANDLE;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    std::vector<std::unique_ptr<VulkanBuffer>> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;
    std::unique_ptr<VulkanDescriptorSet> descriptorSet;

    std::unique_ptr<Texture> texture;

    VkDescriptorSetLayout textureSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool textureDescriptorPool = VK_NULL_HANDLE;

    struct TextureResource {
        std::unique_ptr<Texture> texture;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };
    std::map<std::string, TextureResource> textureCache;
    TextureResource defaultTextureResource;

    void CreateTextureDescriptorSetLayout();
    void CreateTextureDescriptorPool();
    void CreateDefaultTexture();
    VkDescriptorSet GetTextureDescriptorSet(const std::string& path);

    void CreateRenderPass();
    void CreateShadowPass(); // New helper
    void CreatePipeline();
    void CreateOffScreenResources();
    void CreateCommandBuffer();
    void CreateSyncObjects();
    void CreateUniformBuffers();

    void RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, uint32_t currentFrame, Scene& scene, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);

    // Split rendering logic
    // If 'skipIfNotCastingShadow' is true then objects with castsShadow == false are skipped.
    void DrawSceneObjects(VkCommandBuffer cmd, Scene& scene, VkPipelineLayout layout, bool bindTextures, bool skipIfNotCastingShadow = false);
    void RenderShadowMap(VkCommandBuffer cmd, uint32_t currentFrame, Scene& scene);
    void RenderScene(VkCommandBuffer cmd, uint32_t currentFrame, Scene& scene, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);

    void CopyOffScreenToSwapChain(VkCommandBuffer cmd, uint32_t imageIndex);
    void CleanupOffScreenResources();

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    bool framebufferResized = false;
};
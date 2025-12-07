#pragma once

#include "Scene.h"
#include "../vulkan/VulkanDevice.h"
#include "../vulkan/VulkanSwapChain.h"
#include "../vulkan/VulkanRenderPass.h"
#include "../vulkan/VulkanCommandBuffer.h"
#include "../vulkan/VulkanSyncObjects.h"
#include "../vulkan/VulkanDescriptorSet.h"
#include "../vulkan/UniformBufferObject.h"
#include "../pipelines/GraphicsPipeline.h"

#include <memory>

class Renderer {
public:
    Renderer(VulkanDevice* device, VulkanSwapChain* swapChain);
    ~Renderer();

    void Initialize();
    void DrawFrame(Scene& scene, uint32_t currentFrame);
    void UpdateUniformBuffer(uint32_t currentFrame, const UniformBufferObject& ubo);
    void WaitIdle();
    void Cleanup();

    // Accessors for recreation
    VulkanRenderPass* GetRenderPass() { return renderPass.get(); }
    GraphicsPipeline* GetPipeline() { return graphicsPipeline.get(); }


private:
    // Vulkan components (not owned)
    VulkanDevice* device;
    VulkanSwapChain* swapChain;

    // Rendering resources (owned)
    std::unique_ptr<VulkanRenderPass> renderPass;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;
    std::unique_ptr<VulkanCommandBuffer> commandBuffer;
    std::unique_ptr<VulkanSyncObjects> syncObjects;

    // Off-screen rendering
    VkImage offScreenImage = VK_NULL_HANDLE;
    VkDeviceMemory offScreenImageMemory = VK_NULL_HANDLE;
    VkImageView offScreenImageView = VK_NULL_HANDLE;

    // Uniform buffers
    std::vector<std::unique_ptr<VulkanBuffer>> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;
    std::unique_ptr<VulkanDescriptorSet> descriptorSet;

    // Helper functions
    void CreateRenderPass();
    void CreatePipeline();
    void CreateOffScreenResources();
    void CreateCommandBuffer();
    void CreateSyncObjects();
    void CreateUniformBuffers();
    void CreateDescriptorSets();

    void RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, uint32_t currentFrame, Scene& scene);
    void RenderScene(VkCommandBuffer cmd, uint32_t currentFrame, Scene& scene);
    void CopyOffScreenToSwapChain(VkCommandBuffer cmd, uint32_t imageIndex);

    void CleanupOffScreenResources();

    void createImage(uint32_t width, uint32_t height, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image,
        VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    bool framebufferResized = false;
};
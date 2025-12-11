#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <glm/glm.hpp>
#include "../rendering/GraphicsPipeline.h"
#include "../vulkan/VulkanDevice.h"
#include "../vulkan/VulkanUtils.h" // Include the Utils header

class ShadowPass {
public:
    ShadowPass(VulkanDevice* device, uint32_t width, uint32_t height);
    ~ShadowPass();

    void Initialize(VkDescriptorSetLayout globalSetLayout);
    void Cleanup();

    // Commands to record the shadow pass
    void Begin(VkCommandBuffer cmd);
    void End(VkCommandBuffer cmd);

    // Accessors for the Main Renderer
    VkImageView GetShadowImageView() const { return shadowImageView; }
    VkSampler GetShadowSampler() const { return shadowSampler; }
    VkRenderPass GetRenderPass() const { return renderPass; }
    GraphicsPipeline* GetPipeline() const { return pipeline.get(); }
    VkExtent2D GetExtent() const { return { width, height }; }

private:
    VulkanDevice* device;
    uint32_t width;
    uint32_t height;

    // Resources
    VkImage shadowImage = VK_NULL_HANDLE;
    VkDeviceMemory shadowImageMemory = VK_NULL_HANDLE;
    VkImageView shadowImageView = VK_NULL_HANDLE;
    VkSampler shadowSampler = VK_NULL_HANDLE;

    // Render Pipeline
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    std::unique_ptr<GraphicsPipeline> pipeline;

    // Helpers
    void CreateResources();
    void CreateRenderPass();
    void CreateFramebuffer();
    void CreatePipeline(VkDescriptorSetLayout globalSetLayout);

};
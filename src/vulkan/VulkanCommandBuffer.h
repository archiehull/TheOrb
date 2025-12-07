#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

class VulkanCommandBuffer {
public:
    VulkanCommandBuffer(VkDevice device, VkPhysicalDevice physicalDevice);
    ~VulkanCommandBuffer();

    void CreateCommandPool(uint32_t queueFamilyIndex);
    void CreateCommandBuffers(size_t count);
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer,
        VkRenderPass renderPass, VkExtent2D extent,
        VkPipeline pipeline, VkPipelineLayout pipelineLayout);
    void RecordOffScreenCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer,
        VkRenderPass renderPass, VkExtent2D extent,
        VkPipeline pipeline, VkPipelineLayout pipelineLayout);
    void Cleanup();

    VkCommandPool GetCommandPool() const { return commandPool; }
    const std::vector<VkCommandBuffer>& GetCommandBuffers() const { return commandBuffers; }
    VkCommandBuffer GetCommandBuffer(size_t index) const { return commandBuffers[index]; }

    // Single time command helpers
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue);

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
};
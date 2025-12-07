#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanShader.h"
#include <memory>
#include <vector>

class VulkanPipeline {
public:
    VulkanPipeline(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D swapChainExtent, VkFormat swapChainImageFormat);
    ~VulkanPipeline();

    void CreateRenderPass(bool offScreen = false);
    void CreateGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath);
    void CreateFramebuffers(const std::vector<VkImageView>& swapChainImageViews);
    void CreateOffScreenResources();
    void Cleanup();

    VkPipeline GetPipeline() const { return graphicsPipeline; }
    VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
    VkRenderPass GetRenderPass() const { return renderPass; }
    const std::vector<VkFramebuffer>& GetFramebuffers() const { return framebuffers; }

    // Off-screen rendering accessors
    VkImage GetOffScreenImage() const { return offScreenImage; }
    VkImageView GetOffScreenImageView() const { return offScreenImageView; }
    VkFramebuffer GetOffScreenFramebuffer() const { return offScreenFramebuffer; }

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkExtent2D swapChainExtent;
    VkFormat swapChainImageFormat;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;

    // Off-screen rendering resources
    VkImage offScreenImage = VK_NULL_HANDLE;
    VkDeviceMemory offScreenImageMemory = VK_NULL_HANDLE;
    VkImageView offScreenImageView = VK_NULL_HANDLE;
    VkFramebuffer offScreenFramebuffer = VK_NULL_HANDLE;

    std::unique_ptr<VulkanShader> shader;

    // Dynamic states
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    // Helper functions
    void createImage(uint32_t width, uint32_t height, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image,
        VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
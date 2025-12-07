#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanShader.h"
#include <memory>

class VulkanPipeline {
public:
    VulkanPipeline(VkDevice device, VkExtent2D swapChainExtent, VkFormat swapChainImageFormat);
    ~VulkanPipeline();

    void CreateRenderPass();
    void CreateGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath);
    void Cleanup();

    VkPipeline GetPipeline() const { return graphicsPipeline; }
    VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
    VkRenderPass GetRenderPass() const { return renderPass; }

private:
    VkDevice device;
    VkExtent2D swapChainExtent;
    VkFormat swapChainImageFormat;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    std::unique_ptr<VulkanShader> shader;
};
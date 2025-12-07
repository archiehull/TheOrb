#pragma once

#include "Window.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanDevice.h"
#include "vulkan/VulkanSwapChain.h"
#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanCommandBuffer.h"
#include "vulkan/VulkanSyncObjects.h"
#include <memory>
#include <stdexcept>

class Application {
public:
    Application();
    ~Application();

    void Run();

private:
    void InitVulkan();
    void MainLoop();
    void DrawFrame();
    void RenderOffScreen(VkCommandBuffer commandBuffer);
    void CopyOffScreenToSwapChain(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void Cleanup();

    std::unique_ptr<Window> window;
    std::unique_ptr<VulkanContext> vulkanContext;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<VulkanSwapChain> vulkanSwapChain;
    std::unique_ptr<VulkanPipeline> vulkanPipeline;
    std::unique_ptr<VulkanCommandBuffer> vulkanCommandBuffer;
    std::unique_ptr<VulkanSyncObjects> vulkanSyncObjects;

    uint32_t currentFrame = 0;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};
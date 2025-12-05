#pragma once

#include "Window.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanDevice.h"
#include "vulkan/VulkanSwapChain.h"
#include <memory>

class Application {
public:
    Application();
    ~Application();

    void Run();

private:
    void InitVulkan();
    void MainLoop();
    void Cleanup();

    std::unique_ptr<Window> window;
    std::unique_ptr<VulkanContext> vulkanContext;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<VulkanSwapChain> vulkanSwapChain;
};
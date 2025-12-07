#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

class VulkanSyncObjects {
public:
    VulkanSyncObjects(VkDevice device, size_t maxFramesInFlight = 2);
    ~VulkanSyncObjects();

    void CreateSyncObjects(size_t swapChainImageCount);
    void Cleanup();

    VkSemaphore GetImageAvailableSemaphore(size_t frame) const { return imageAvailableSemaphores[frame]; }
    VkSemaphore GetRenderFinishedSemaphore(size_t imageIndex) const { return renderFinishedSemaphores[imageIndex]; }
    VkFence GetInFlightFence(size_t frame) const { return inFlightFences[frame]; }

    size_t GetMaxFramesInFlight() const { return maxFramesInFlight; }

private:
    VkDevice device;
    size_t maxFramesInFlight;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores; // One per swap chain image
    std::vector<VkFence> inFlightFences;
};
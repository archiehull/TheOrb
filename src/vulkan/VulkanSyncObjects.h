#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanSyncObjects {
public:
    VulkanSyncObjects(VkDevice device, uint32_t maxFramesInFlight);
    ~VulkanSyncObjects();

    void CreateSyncObjects(uint32_t swapChainImageCount);
    void Cleanup();

    // Per-swapchain-image semaphores (indexed by imageIndex)
    VkSemaphore GetImageAvailableSemaphore(uint32_t imageIndex) const;
    VkSemaphore GetRenderFinishedSemaphore(uint32_t imageIndex) const;

    // Per-frame-in-flight fences (indexed by currentFrame)
    VkFence GetInFlightFence(uint32_t currentFrame) const;

    // Track which fence is using which image
    VkFence& GetImageInFlight(uint32_t imageIndex);

private:
    VkDevice device;
    uint32_t maxFramesInFlight;

    // One semaphore per swapchain image (usually 2-3)
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    // One fence per frame in flight (2)
    std::vector<VkFence> inFlightFences;

    // Track which frame's fence is using which swapchain image
    std::vector<VkFence> imagesInFlight;
};
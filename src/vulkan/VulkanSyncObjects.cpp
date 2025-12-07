#include "VulkanSyncObjects.h"
#include <stdexcept>

VulkanSyncObjects::VulkanSyncObjects(VkDevice device, uint32_t maxFramesInFlight)
    : device(device), maxFramesInFlight(maxFramesInFlight) {
}

VulkanSyncObjects::~VulkanSyncObjects() {
}

void VulkanSyncObjects::CreateSyncObjects(uint32_t swapChainImageCount) {
    // Create semaphores for EACH swapchain image (solution from validation layer)
    imageAvailableSemaphores.resize(swapChainImageCount);
    renderFinishedSemaphores.resize(swapChainImageCount);

    // Create fences for frames in flight
    inFlightFences.resize(maxFramesInFlight);

    // Track which fence is using which image
    imagesInFlight.resize(swapChainImageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create one semaphore pair per swapchain image
    for (size_t i = 0; i < swapChainImageCount; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores for swapchain image!");
        }
    }

    // Create fences for frames in flight
    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fence for frame in flight!");
        }
    }
}

void VulkanSyncObjects::Cleanup() {
    // Cleanup semaphores (per swapchain image)
    for (auto semaphore : imageAvailableSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }
    imageAvailableSemaphores.clear();

    for (auto semaphore : renderFinishedSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }
    renderFinishedSemaphores.clear();

    // Cleanup fences (per frame in flight)
    for (auto fence : inFlightFences) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(device, fence, nullptr);
        }
    }
    inFlightFences.clear();

    imagesInFlight.clear();
}

VkSemaphore VulkanSyncObjects::GetImageAvailableSemaphore(uint32_t imageIndex) const {
    return imageAvailableSemaphores[imageIndex];
}

VkSemaphore VulkanSyncObjects::GetRenderFinishedSemaphore(uint32_t imageIndex) const {
    return renderFinishedSemaphores[imageIndex];
}

VkFence VulkanSyncObjects::GetInFlightFence(uint32_t currentFrame) const {
    return inFlightFences[currentFrame];
}

VkFence& VulkanSyncObjects::GetImageInFlight(uint32_t imageIndex) {
    return imagesInFlight[imageIndex];
}
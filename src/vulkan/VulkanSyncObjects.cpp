#include "VulkanSyncObjects.h"
#include <stdexcept>

VulkanSyncObjects::VulkanSyncObjects(VkDevice device, size_t maxFramesInFlight)
    : device(device), maxFramesInFlight(maxFramesInFlight) {
}

VulkanSyncObjects::~VulkanSyncObjects() {
}

void VulkanSyncObjects::CreateSyncObjects(size_t swapChainImageCount) {
    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(swapChainImageCount); // One per swap chain image
    inFlightFences.resize(maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    // Create one render finished semaphore per swap chain image
    for (size_t i = 0; i < swapChainImageCount; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render finished semaphore!");
        }
    }
}

void VulkanSyncObjects::Cleanup() {
    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        }
        if (inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
    }

    for (auto semaphore : renderFinishedSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }
}
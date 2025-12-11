#pragma once

#include <vulkan/vulkan.h>
#include "VulkanUtils.h"

class VulkanBuffer {
public:
    VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice);
    ~VulkanBuffer();

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    void CopyData(const void* data, VkDeviceSize size);

    VkBuffer GetBuffer() const { return buffer; }
    VkDeviceMemory GetBufferMemory() const { return bufferMemory; }

    void Cleanup();

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;

};
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <vulkan/vulkan_core.h>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanDevice {
public:
    VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
    ~VulkanDevice();

    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void Cleanup();

    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
    VkDevice GetDevice() const { return device; }
    VkQueue GetGraphicsQueue() const { return graphicsQueue; }
    VkQueue GetPresentQueue() const { return presentQueue; }
    QueueFamilyIndices GetQueueFamilies() const { return findQueueFamilies(physicalDevice); }

private:
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    bool isDeviceSuitable(VkPhysicalDevice device) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
};
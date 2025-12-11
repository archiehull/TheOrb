#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "../vulkan/VulkanUtils.h" 

class Texture {
public:
    Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    ~Texture();

    // Loads file via stb_image, uploads to GPU, creates image view + sampler.
    bool LoadFromFile(const std::string& filepath);

    VkImageView GetImageView() const { return imageView; }
    VkSampler GetSampler() const { return sampler; }
    VkImage GetImage() const { return image; }

    void Cleanup();

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;

    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

};
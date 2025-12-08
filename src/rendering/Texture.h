#pragma once

#include <vulkan/vulkan.h>
#include <string>

class Texture {
public:
    Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    ~Texture();

    // Loads file via stb_image, uploads to GPU, creates image view + sampler (anisotropic if supported).
    // Returns false on failure.
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

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    void transitionImageLayout(VkImage img, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage img, uint32_t width, uint32_t height);
};
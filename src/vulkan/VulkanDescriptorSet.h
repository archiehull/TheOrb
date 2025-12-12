#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanDescriptorSet {
public:
    VulkanDescriptorSet(VkDevice device);
    ~VulkanDescriptorSet();

    void CreateDescriptorSetLayout();
    void CreateDescriptorPool(uint32_t maxSets);
    // Update the signature to accept skybox resources
    void CreateDescriptorSets(const std::vector<VkBuffer>& uniformBuffers, VkDeviceSize bufferSize,
        VkImageView shadowImageView, VkSampler shadowSampler,
        VkImageView skyboxImageView, VkSampler skyboxSampler);
    void Cleanup();

    VkDescriptorSetLayout GetLayout() const { return descriptorSetLayout; }
    VkDescriptorSet GetDescriptorSet(uint32_t index) const { return descriptorSets[index]; }
    const std::vector<VkDescriptorSet>& GetDescriptorSets() const { return descriptorSets; }

private:
    VkDevice device;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;
};
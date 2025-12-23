#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanRenderPass {
public:
    VulkanRenderPass(VkDevice device, VkFormat swapChainImageFormat);
    ~VulkanRenderPass() = default;

    void Create(bool offScreen = false);
    void CreateFramebuffers(const std::vector<VkImageView>& imageViews, const VkExtent2D& extent);
    void CreateOffScreenFramebuffer(VkImageView colorImageView, VkImageView depthImageView, const VkExtent2D& extent);
    void Cleanup();

    VkRenderPass GetRenderPass() const { return renderPass; }
    const std::vector<VkFramebuffer>& GetFramebuffers() const { return framebuffers; }
    VkFramebuffer GetOffScreenFramebuffer() const { return offScreenFramebuffer; }

private:
    VkDevice device;
    VkFormat imageFormat;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkFramebuffer offScreenFramebuffer = VK_NULL_HANDLE;
};
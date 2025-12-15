#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "Cubemap.h"
#include "GraphicsPipeline.h"
#include "Scene.h"

class SkyboxPass {
public:
    SkyboxPass(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    ~SkyboxPass();

    void Initialize(VkRenderPass renderPass, VkExtent2D extent, VkDescriptorSetLayout globalSetLayout);
    void Draw(VkCommandBuffer cmd, Scene& scene, uint32_t currentFrame, VkDescriptorSet globalDescriptorSet);
    void Cleanup();

    Cubemap* GetCubemap() const { return cubemap.get(); }

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;

    std::unique_ptr<Cubemap> cubemap;
    std::unique_ptr<GraphicsPipeline> pipeline;

    std::vector<std::string> GetSkyboxFaces(const std::string& name);
};
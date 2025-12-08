#pragma once

#include "Pipeline.h"
#include "../vulkan/VulkanShader.h"
#include "../vulkan/Vertex.h"
#include <memory>
#include <vector>

struct GraphicsPipelineConfig {
    std::string vertShaderPath;
    std::string fragShaderPath;
    VkRenderPass renderPass;
    VkExtent2D extent;

    // Vertex input
    VkVertexInputBindingDescription* bindingDescription = nullptr;
    VkVertexInputAttributeDescription* attributeDescriptions = nullptr;
    uint32_t attributeCount = 0;

    // Descriptor set layout (for uniforms)
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

    // Rasterization
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    float lineWidth = 1.0f;

    // Multisampling
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

    // Depth testing
    bool depthTestEnable = false;
    bool depthWriteEnable = false;

    // Blending
    bool blendEnable = false;
};

class GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline(VkDevice device, const GraphicsPipelineConfig& config);
    ~GraphicsPipeline();

    void Create() override;
    void Cleanup() override;

private:
    GraphicsPipelineConfig config;
    std::unique_ptr<VulkanShader> shader;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
};
#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include "../vulkan/Vertex.h"
#include "../vulkan/VulkanShader.h" // Needed for the shader member

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
    bool depthBiasEnable = false; // Added missing config field from previous context

    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

    // Blending
    bool blendEnable = false;
};

class GraphicsPipeline {
public:
    GraphicsPipeline(VkDevice device, const GraphicsPipelineConfig& config);
    ~GraphicsPipeline();

    void Create();
    void Cleanup();

    // Accessors (Previously in Pipeline base)
    VkPipeline GetPipeline() const { return pipeline; }
    VkPipelineLayout GetLayout() const { return pipelineLayout; }

private:
    VkDevice device;
    GraphicsPipelineConfig config;

    // Core Vulkan objects (Moved from Pipeline base)
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    std::unique_ptr<VulkanShader> shader;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
};
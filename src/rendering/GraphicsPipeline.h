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
    uint32_t bindingCount = 1;
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

    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
};

class GraphicsPipeline final {
public:
    GraphicsPipeline(VkDevice deviceArg, const GraphicsPipelineConfig& configArg);
    ~GraphicsPipeline();

    // Non-copyable
    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    // Movable
    GraphicsPipeline(GraphicsPipeline&&) noexcept = default;
    GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept = default;

    void Create();
    void Cleanup();

    // Accessors (Previously in Pipeline base)
    VkPipeline GetPipeline() const { return pipeline; }
    VkPipelineLayout GetLayout() const { return pipelineLayout; }

private:
    VkDevice device;

    // Keep frequently accessed small handles together first for better layout
    std::unique_ptr<VulkanShader> shader;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    // Dynamic states can change at runtime
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    // Larger config struct last (contains std::vector)
    GraphicsPipelineConfig config;
};
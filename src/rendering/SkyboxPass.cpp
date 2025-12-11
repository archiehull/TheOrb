#include "SkyboxPass.h"
#include "../vulkan/Vertex.h"
#include "../vulkan/PushConstantObject.h"

SkyboxPass::SkyboxPass(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue)
    : device(device), physicalDevice(physicalDevice), commandPool(commandPool), graphicsQueue(graphicsQueue) {
}

SkyboxPass::~SkyboxPass() {
    Cleanup();
}

void SkyboxPass::Initialize(VkRenderPass renderPass, VkExtent2D extent, VkDescriptorSetLayout globalSetLayout) {
    // 1. Initialize Cubemap
    cubemap = std::make_unique<Cubemap>(device, physicalDevice, commandPool, graphicsQueue);

    std::vector<std::string> faces = {
        "textures/skybox/cubemap_0(+X).jpg", "textures/skybox/cubemap_1(-X).jpg",
        "textures/skybox/cubemap_2(+Y).jpg", "textures/skybox/cubemap_3(-Y).jpg",
        "textures/skybox/cubemap_4(+Z).jpg", "textures/skybox/cubemap_5(-Z).jpg"
    };
    cubemap->LoadFromFiles(faces);

    // 2. Create Pipeline
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    GraphicsPipelineConfig config{};
    // Ensure these point to your compiled SPIR-V shaders
    config.vertShaderPath = "src/shaders/skybox_vert.spv";
    config.fragShaderPath = "src/shaders/skybox_frag.spv";

    config.renderPass = renderPass;
    config.extent = extent;
    config.bindingDescription = &bindingDescription;
    config.attributeDescriptions = attributeDescriptions.data();

    // FIX: Only use the first attribute (Position)
    // The Vertex struct has 4 attributes (Pos, Color, UV, Normal), but skybox.vert only uses Location 0.
    // This prevents the "Vertex attribute at location X not consumed" validation error.
    config.attributeCount = 1;

    // Layout: Set 0 = Global UBO, Set 1 = Cubemap
    config.descriptorSetLayouts = { globalSetLayout, cubemap->GetDescriptorSetLayout() };

    // Settings for "Crystal Ball" (viewable from inside)
    config.cullMode = VK_CULL_MODE_FRONT_BIT;
    config.depthTestEnable = true;
    config.depthWriteEnable = true;

    pipeline = std::make_unique<GraphicsPipeline>(device, config);
    pipeline->Create();
}

void SkyboxPass::Draw(VkCommandBuffer cmd, Scene& scene, uint32_t currentFrame, VkDescriptorSet globalDescriptorSet) {
    if (!pipeline || !cubemap) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

    // Bind Global UBO (Set 0)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, &globalDescriptorSet, 0, nullptr);

    // Bind Cubemap (Set 1)
    VkDescriptorSet skySet = cubemap->GetDescriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 1, 1, &skySet, 0, nullptr);

    // Render only objects marked with shadingMode = 2 (Skybox Mode)
    for (const auto& obj : scene.GetObjects()) {
        if (!obj->visible || !obj->geometry || obj->shadingMode != 2) continue;

        PushConstantObject pco{};
        pco.model = obj->transform;
        pco.shadingMode = 2;

        vkCmdPushConstants(cmd, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantObject), &pco);

        obj->geometry->Bind(cmd);
        obj->geometry->Draw(cmd);
    }
}

void SkyboxPass::Cleanup() {
    if (pipeline) {
        pipeline->Cleanup();
        pipeline.reset();
    }
    if (cubemap) {
        cubemap->Cleanup();
        cubemap.reset();
    }
}
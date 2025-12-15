#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "Renderer.h"
#include "../vulkan/Vertex.h"
#include "../vulkan/VulkanUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <iostream>

Renderer::Renderer(VulkanDevice* device, VulkanSwapChain* swapChain)
    : device(device), swapChain(swapChain) {
}

Renderer::~Renderer() {
}

void Renderer::Initialize() {
    CreateRenderPass();

    // 1. Main Offscreen Framebuffer (Color + Depth)
    CreateOffScreenResources();
    renderPass->CreateOffScreenFramebuffer(offScreenImageView, depthImageView, swapChain->GetExtent());

    // 2. Refraction Framebuffer
    {
        std::array<VkImageView, 2> attachments = {
            refractionImageView,
            depthImageView // Reuse depth buffer
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass->GetRenderPass();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChain->GetExtent().width;
        framebufferInfo.height = swapChain->GetExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device->GetDevice(), &framebufferInfo, nullptr, &refractionFramebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create refraction framebuffer!");
        }
    }

    CreateUniformBuffers();
    CreateCommandBuffer();

    CreateTextureDescriptorSetLayout();
    CreateTextureDescriptorPool();
    CreateDefaultTexture();

    // Global Descriptor Set (UBOs)
    descriptorSet = std::make_unique<VulkanDescriptorSet>(device->GetDevice());
    descriptorSet->CreateDescriptorSetLayout();

    // Initialize Skybox
    skyboxPass = std::make_unique<SkyboxPass>(
        device->GetDevice(), device->GetPhysicalDevice(),
        commandBuffer->GetCommandPool(), device->GetGraphicsQueue()
    );
    skyboxPass->Initialize(renderPass->GetRenderPass(), swapChain->GetExtent(), descriptorSet->GetLayout());

    CreateShadowPass();

    // Create Descriptor Sets
    descriptorSet->CreateDescriptorPool(MAX_FRAMES_IN_FLIGHT);

    std::vector<VkBuffer> buffers;
    for (const auto& uniformBuffer : uniformBuffers) buffers.push_back(uniformBuffer->GetBuffer());

    descriptorSet->CreateDescriptorSets(
        buffers,
        sizeof(UniformBufferObject),
        shadowPass->GetShadowImageView(),
        shadowPass->GetShadowSampler(),
        refractionImageView,
        refractionSampler
    );

    // --- Create Shared Particle Pipelines ---
    CreateParticlePipelines();

    CreatePipeline(); // Main scene object pipeline
    CreateSyncObjects();
}

void Renderer::DrawFrame(Scene& scene, uint32_t currentFrame, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    // Wait for this frame's fence
    VkFence fence = syncObjects->GetInFlightFence(currentFrame);
    vkWaitForFences(device->GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

    // Acquire next image
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device->GetDevice(),
        swapChain->GetSwapChain(),
        UINT64_MAX,
        syncObjects->GetImageAvailableSemaphore(currentFrame),
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Check if this image is already in use
    VkFence& imageInFlightFence = syncObjects->GetImageInFlight(imageIndex);
    if (imageInFlightFence != VK_NULL_HANDLE) {
        vkWaitForFences(device->GetDevice(), 1, &imageInFlightFence, VK_TRUE, UINT64_MAX);
    }
    imageInFlightFence = fence;

    vkResetFences(device->GetDevice(), 1, &fence);

    VkCommandBuffer cmd = commandBuffer->GetCommandBuffer(currentFrame);
    RecordCommandBuffer(cmd, imageIndex, currentFrame, scene, viewMatrix, projMatrix);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { syncObjects->GetImageAvailableSemaphore(currentFrame) };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = { syncObjects->GetRenderFinishedSemaphore(imageIndex) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain->GetSwapChain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(device->GetPresentQueue(), &presentInfo);
}

void Renderer::CreateShadowPass() {
    // Shadow resolution typically higher than screen, e.g., 2048 or 4096
    shadowPass = std::make_unique<ShadowPass>(device, 2048, 2048);
    // Initialize passing the Global UBO Layout (Set 0)
    shadowPass->Initialize(descriptorSet->GetLayout());
}

void Renderer::CreateUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i] = std::make_unique<VulkanBuffer>(device->GetDevice(), device->GetPhysicalDevice());
        uniformBuffers[i]->CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkMapMemory(device->GetDevice(), uniformBuffers[i]->GetBufferMemory(), 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void Renderer::CreateTextureDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0; // Binding 0 in Set 1
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(device->GetDevice(), &layoutInfo, nullptr, &textureSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture set layout!");
    }
}

void Renderer::CreateTextureDescriptorPool() {
    // Allow for a reasonable number of textures
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 100;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 100;

    if (vkCreateDescriptorPool(device->GetDevice(), &poolInfo, nullptr, &textureDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture descriptor pool!");
    }
}

void Renderer::CreateDefaultTexture() {
    defaultTextureResource.texture = std::make_unique<Texture>(
        device->GetDevice(), device->GetPhysicalDevice(),
        commandBuffer->GetCommandPool(), device->GetGraphicsQueue());

    // load default.png
    defaultTextureResource.texture->LoadFromFile("textures/default.png");

    // Create Descriptor Set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = textureDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureSetLayout;

    vkAllocateDescriptorSets(device->GetDevice(), &allocInfo, &defaultTextureResource.descriptorSet);

    // Update Descriptor Set
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = defaultTextureResource.texture->GetImageView();
    imageInfo.sampler = defaultTextureResource.texture->GetSampler();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = defaultTextureResource.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device->GetDevice(), 1, &descriptorWrite, 0, nullptr);
}

VkDescriptorSet Renderer::GetTextureDescriptorSet(const std::string& path) {
    if (path.empty()) return defaultTextureResource.descriptorSet;

    // Check cache
    auto it = textureCache.find(path);
    if (it != textureCache.end()) {
        return it->second.descriptorSet;
    }

    // Load new texture
    auto texture = std::make_unique<Texture>(
        device->GetDevice(), device->GetPhysicalDevice(),
        commandBuffer->GetCommandPool(), device->GetGraphicsQueue());

    if (!texture->LoadFromFile(path)) {
        std::cerr << "Failed to load texture: " << path << ", using default." << std::endl;
        return defaultTextureResource.descriptorSet;
    }

    // Allocate Set
    VkDescriptorSet descSet;
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = textureDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureSetLayout;

    vkAllocateDescriptorSets(device->GetDevice(), &allocInfo, &descSet);

    // Update Set
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->GetImageView();
    imageInfo.sampler = texture->GetSampler();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device->GetDevice(), 1, &descriptorWrite, 0, nullptr);

    // Store in cache
    textureCache[path] = { std::move(texture), descSet };
    return descSet;
}

void Renderer::UpdateUniformBuffer(uint32_t currentFrame, const UniformBufferObject& ubo) {
    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

static bool HasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates,
    VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
    return findSupportedFormat(physicalDevice,
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void Renderer::CreateRenderPass() {
    renderPass = std::make_unique<VulkanRenderPass>(
        device->GetDevice(),
        swapChain->GetImageFormat()
    );
    renderPass->Create(true); // off-screen rendering
}

void Renderer::CreatePipeline() {
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    GraphicsPipelineConfig pipelineConfig{};
    pipelineConfig.vertShaderPath = "src/shaders/vert.spv";
    pipelineConfig.fragShaderPath = "src/shaders/frag.spv";
    pipelineConfig.renderPass = renderPass->GetRenderPass();
    pipelineConfig.extent = swapChain->GetExtent();
    pipelineConfig.bindingDescription = &bindingDescription;
    pipelineConfig.attributeDescriptions = attributeDescriptions.data();
    pipelineConfig.attributeCount = static_cast<uint32_t>(attributeDescriptions.size());
    pipelineConfig.descriptorSetLayouts = {
        descriptorSet->GetLayout(),
        textureSetLayout
    };

    // Main pipeline standard settings
    pipelineConfig.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineConfig.depthTestEnable = true;
    pipelineConfig.depthWriteEnable = true;

    pipelineConfig.blendEnable = true;

    graphicsPipeline = std::make_unique<GraphicsPipeline>(device->GetDevice(), pipelineConfig);
    graphicsPipeline->Create();
}

void Renderer::CreateOffScreenResources() {
    VkExtent2D extent = swapChain->GetExtent();
    VkFormat imageFormat = swapChain->GetImageFormat();

    // --- 1. Main OffScreen Color Attachment ---
    VulkanUtils::CreateImage(
        device->GetDevice(),
        device->GetPhysicalDevice(),
        extent.width, extent.height, 1, 1,
        imageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        // MUST include TRANSFER_SRC_BIT because the render pass finalLayout is TRANSFER_SRC_OPTIMAL
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        offScreenImage,
        offScreenImageMemory
    );

    offScreenImageView = VulkanUtils::CreateImageView(
        device->GetDevice(), offScreenImage, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT
    );

    // --- 2. Refraction Color Attachment ---
    // Ensure transfer src bit is present (render pass may transition to TRANSFER_SRC_OPTIMAL)
    VulkanUtils::CreateImage(
        device->GetDevice(),
        device->GetPhysicalDevice(),
        extent.width, extent.height, 1, 1,
        imageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        refractionImage,
        refractionImageMemory
    );

    refractionImageView = VulkanUtils::CreateImageView(
        device->GetDevice(), refractionImage, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT
    );

    // Create Sampler for Refraction
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device->GetDevice(), &samplerInfo, nullptr, &refractionSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create refraction sampler!");
    }

    // --- 3. Shared Depth Attachment ---
    VkFormat depthFormat = findDepthFormat(device->GetPhysicalDevice());

    VulkanUtils::CreateImage(
        device->GetDevice(),
        device->GetPhysicalDevice(),
        extent.width, extent.height, 1, 1,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage,
        depthImageMemory
    );

    depthImageView = VulkanUtils::CreateImageView(
        device->GetDevice(), depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT
    );
}

void Renderer::RenderRefractionPass(VkCommandBuffer cmd, uint32_t currentFrame, Scene& scene) {
    // 1. Begin Render Pass targeting the Refraction Framebuffer
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass->GetRenderPass();
    renderPassInfo.framebuffer = refractionFramebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChain->GetExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.1f, 0.1f, 0.1f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.width = (float)swapChain->GetExtent().width;
    viewport.height = (float)swapChain->GetExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = swapChain->GetExtent();
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // 2. Draw Skybox
    if (skyboxPass) {
        skyboxPass->Draw(cmd, scene, currentFrame, descriptorSet->GetDescriptorSets()[currentFrame]);
    }

    // 3. Draw Scene Objects (Excluding Refractive objects)
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetPipeline());
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetLayout(), 0, 1, &descriptorSet->GetDescriptorSets()[currentFrame], 0, nullptr);

    for (const auto& obj : scene.GetObjects()) {
        if (!obj || !obj->visible || !obj->geometry || obj->shadingMode == 3 || obj->shadingMode == 2 || obj->shadingMode == 4) continue;
        PushConstantObject pco{};
        pco.model = obj->transform;
        pco.shadingMode = obj->shadingMode;
        vkCmdPushConstants(cmd, graphicsPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantObject), &pco);

        VkDescriptorSet textureSet = GetTextureDescriptorSet(obj->texturePath);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetLayout(), 1, 1, &textureSet, 0, nullptr);

        obj->geometry->Bind(cmd);
        obj->geometry->Draw(cmd);
    }

    vkCmdEndRenderPass(cmd);

    // 4. Barrier: Synchronize so Main Pass can read this texture
    // FIX: Changed oldLayout to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    // The RenderPass automatically transitions the attachment to its 'finalLayout' when it ends.
    // Since we reused the offscreen render pass, that final layout is TRANSFER_SRC_OPTIMAL.
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = refractionImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // We are waiting for Color Attachment Writes to finish (from the render pass)
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // We want to read it in the Fragment Shader in the next pass
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void Renderer::CreateCommandBuffer() {
    commandBuffer = std::make_unique<VulkanCommandBuffer>(
        device->GetDevice(),
        device->GetPhysicalDevice()
    );
    commandBuffer->CreateCommandPool(device->GetQueueFamilies().graphicsFamily.value());
    commandBuffer->CreateCommandBuffers(MAX_FRAMES_IN_FLIGHT);
}

void Renderer::CreateSyncObjects() {
    syncObjects = std::make_unique<VulkanSyncObjects>(
        device->GetDevice(),
        MAX_FRAMES_IN_FLIGHT
    );

    // Explicitly cast size_t -> uint32_t and guard against empty swapchain
    uint32_t imageCount = static_cast<uint32_t>(swapChain->GetImages().size());
    if (imageCount == 0) {
        throw std::runtime_error("swap chain contains no images");
    }

    // Initialize sync objects (create semaphores/fences) using swapchain image count
    syncObjects->CreateSyncObjects(imageCount);
}

void Renderer::DrawSceneObjects(VkCommandBuffer cmd, Scene& scene, VkPipelineLayout layout, bool bindTextures, bool skipIfNotCastingShadow) {
    for (const auto& obj : scene.GetObjects()) {
        if (!obj || !obj->visible || !obj->geometry) continue;
        if (skipIfNotCastingShadow && !obj->castsShadow) continue;

        PushConstantObject pco{};
        pco.model = obj->transform;
        pco.shadingMode = obj->shadingMode;

        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantObject), &pco);

        if (bindTextures) {
            VkDescriptorSet textureSet = GetTextureDescriptorSet(obj->texturePath);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &textureSet, 0, nullptr);
        }

        obj->geometry->Bind(cmd);
        obj->geometry->Draw(cmd);
    }
}

void Renderer::CreateParticlePipelines() {
    // 1. Get Vertex Input Info from ParticleSystem (Static Helpers)
    auto bindings = ParticleSystem::GetBindingDescriptions();
    auto attribs = ParticleSystem::GetAttributeDescriptions();

    // 2. Configure the Base Pipeline
    GraphicsPipelineConfig config{};
    config.vertShaderPath = "src/shaders/particle_vert.spv";
    config.fragShaderPath = "src/shaders/particle_frag.spv";
    config.renderPass = renderPass->GetRenderPass();
    config.extent = swapChain->GetExtent();

    config.bindingDescription = bindings.data();
    config.bindingCount = static_cast<uint32_t>(bindings.size());
    config.attributeDescriptions = attribs.data();
    config.attributeCount = static_cast<uint32_t>(attribs.size());

    // Set 0: Global UBO (Camera), Set 1: Particle Texture (Using SHARED textureSetLayout)
    config.descriptorSetLayouts = { descriptorSet->GetLayout(), textureSetLayout };

    // Particles don't write to depth buffer (transparent), but they do test against it.
    config.depthWriteEnable = false;
    config.depthTestEnable = true;
    config.blendEnable = true;

    // --- Variant A: Additive Pipeline (e.g., Fire, Magic) ---
    config.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    config.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

    particlePipelineAdditive = std::make_unique<GraphicsPipeline>(device->GetDevice(), config);
    particlePipelineAdditive->Create();

    // --- Variant B: Alpha Blended Pipeline (e.g., Smoke, Dust) ---
    config.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    config.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    particlePipelineAlpha = std::make_unique<GraphicsPipeline>(device->GetDevice(), config);
    particlePipelineAlpha->Create();
}

void Renderer::SetupSceneParticles(Scene& scene) {
    scene.SetupParticleSystem(
        commandBuffer->GetCommandPool(),
        device->GetGraphicsQueue(),
        particlePipelineAdditive.get(),
        particlePipelineAlpha.get(),
        textureSetLayout,
        MAX_FRAMES_IN_FLIGHT
    );
}

void Renderer::RenderShadowMap(VkCommandBuffer cmd, uint32_t currentFrame, Scene& scene) {
    shadowPass->Begin(cmd);

    // Bind Global UBO (Set 0) - Contains LightSpaceMatrix
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        shadowPass->GetPipeline()->GetLayout(),
        0,
        1,
        &descriptorSet->GetDescriptorSets()[currentFrame],
        0,
        nullptr
    );

    // Draw objects using shadow pipeline layout (No textures needed)
    // Skip objects that should not cast shadows (e.g. the Sun/Moon spheres)
    DrawSceneObjects(cmd, scene, shadowPass->GetPipeline()->GetLayout(), false, true);

    shadowPass->End(cmd);
}

void Renderer::RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex,
    uint32_t currentFrame, Scene& scene,
    const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {

    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // --- 0. Update UBO ---
    glm::vec3 lightPos = glm::vec3(0.0f, 200.0f, 0.0f);
    const auto& lights = scene.GetLights();
    if (!lights.empty()) lightPos = lights[0].position;

    glm::mat4 lightProj = glm::ortho(-200.0f, 200.0f, -200.0f, 200.0f, 1.0f, 500.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    lightProj[1][1] *= -1;
    glm::mat4 lightSpaceMatrix = lightProj * lightView;

    UniformBufferObject ubo{};
    ubo.view = viewMatrix;
    ubo.proj = projMatrix;
    ubo.viewPos = glm::vec3(glm::inverse(viewMatrix)[3]);
    ubo.lightSpaceMatrix = lightSpaceMatrix;
    size_t count = std::min(lights.size(), (size_t)MAX_LIGHTS);
    if (count > 0) std::memcpy(ubo.lights, lights.data(), count * sizeof(Light));
    ubo.numLights = static_cast<int>(count);
    UpdateUniformBuffer(currentFrame, ubo);

    // --- 1. Render Shadow Pass ---
    RenderShadowMap(cmd, currentFrame, scene);

    // --- 2. Render Refraction Pass ---
    RenderRefractionPass(cmd, currentFrame, scene);

    // --- 3. Render Main Scene ---
    RenderScene(cmd, currentFrame, scene);

    // --- 4. Copy to SwapChain ---
    CopyOffScreenToSwapChain(cmd, imageIndex);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Renderer::RenderScene(VkCommandBuffer cmd, uint32_t currentFrame, Scene& scene) {
    // Begin Render Pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass->GetRenderPass();
    renderPassInfo.framebuffer = renderPass->GetOffScreenFramebuffer();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChain->GetExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChain->GetExtent().width;
    viewport.height = (float)swapChain->GetExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChain->GetExtent();
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // --- 1. Skybox Pipeline (Background & Interiors) ---
    if (skyboxPass) {
        skyboxPass->Draw(cmd, scene, currentFrame, descriptorSet->GetDescriptorSets()[currentFrame]);
    }

    // --- 2. Main Pipeline (Standard Objects + Transparent Front Faces) ---
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetPipeline());

    // Bind Global UBO (Set 0) 
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetLayout(), 0, 1, &descriptorSet->GetDescriptorSets()[currentFrame], 0, nullptr);

    for (const auto& obj : scene.GetObjects()) {
        // Skip invisible, empty, OR Skybox objects (Mode 2 is handled by SkyboxPass)
        if (!obj || !obj->visible || !obj->geometry || obj->shadingMode == 2) continue;

        // Push Constants
        PushConstantObject pco{};
        pco.model = obj->transform;
        pco.shadingMode = obj->shadingMode;
        vkCmdPushConstants(cmd, graphicsPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantObject), &pco);

        // Bind Texture (Set 1)
        VkDescriptorSet textureSet = GetTextureDescriptorSet(obj->texturePath);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetLayout(), 1, 1, &textureSet, 0, nullptr);

        // Draw Geometry
        obj->geometry->Bind(cmd);
        obj->geometry->Draw(cmd);
    }

    // --- 3. Dynamic Particle Systems ---
    for (const auto& sys : scene.GetParticleSystems()) {
        sys->Draw(cmd, descriptorSet->GetDescriptorSets()[currentFrame], currentFrame);
    }

    vkCmdEndRenderPass(cmd);
}

void Renderer::CopyOffScreenToSwapChain(VkCommandBuffer cmd, uint32_t imageIndex) {
    // Transition off-screen image to transfer source
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = offScreenImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    // Transition swap chain image to transfer destination
    VkImageMemoryBarrier swapChainBarrier{};
    swapChainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapChainBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapChainBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapChainBarrier.image = swapChain->GetImages()[imageIndex];
    swapChainBarrier.subresourceRange = barrier.subresourceRange;
    swapChainBarrier.srcAccessMask = 0;
    swapChainBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);

    // Copy
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copyRegion.extent = { swapChain->GetExtent().width, swapChain->GetExtent().height, 1 };

    vkCmdCopyImage(cmd,
        offScreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapChain->GetImages()[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copyRegion);

    // Transition swap chain to present
    swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    swapChainBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    swapChainBarrier.dstAccessMask = 0;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);
}

void Renderer::WaitIdle() {
    vkDeviceWaitIdle(device->GetDevice());
}

void Renderer::Cleanup() {
    // 1. Cleanup Scene Uniform Buffers
    for (size_t i = 0; i < uniformBuffers.size(); i++) {
        if (uniformBuffersMapped[i]) {
            vkUnmapMemory(device->GetDevice(), uniformBuffers[i]->GetBufferMemory());
        }
    }

    for (auto& uniformBuffer : uniformBuffers) {
        if (uniformBuffer) {
            uniformBuffer->Cleanup();
        }
    }
    uniformBuffers.clear();
    uniformBuffersMapped.clear();

    if (descriptorSet) {
        descriptorSet->Cleanup();
        descriptorSet.reset();
    }

    // 2. Cleanup Texture Cache
    for (auto& entry : textureCache) {
        if (entry.second.texture) {
            entry.second.texture->Cleanup();
            entry.second.texture.reset();
        }
        entry.second.descriptorSet = VK_NULL_HANDLE;
    }
    textureCache.clear();

    if (defaultTextureResource.texture) {
        defaultTextureResource.texture->Cleanup();
        defaultTextureResource.texture.reset();
    }
    defaultTextureResource.descriptorSet = VK_NULL_HANDLE;

    if (textureDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device->GetDevice(), textureDescriptorPool, nullptr);
        textureDescriptorPool = VK_NULL_HANDLE;
    }

    if (textureSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device->GetDevice(), textureSetLayout, nullptr);
        textureSetLayout = VK_NULL_HANDLE;
    }

    // 3. Cleanup Shared Particle Resources
    if (particlePipelineAdditive) {
        particlePipelineAdditive->Cleanup();
        particlePipelineAdditive.reset();
    }
    if (particlePipelineAlpha) {
        particlePipelineAlpha->Cleanup();
        particlePipelineAlpha.reset();
    }
    // Note: We used textureSetLayout for particles, so no separate particleTextureLayout to destroy.

    // 4. Cleanup Core Renderer Components
    if (syncObjects) {
        syncObjects->Cleanup();
        syncObjects.reset();
    }

    if (commandBuffer) {
        commandBuffer->Cleanup();
        commandBuffer.reset();
    }

    if (graphicsPipeline) {
        graphicsPipeline->Cleanup();
        graphicsPipeline.reset();
    }

    if (shadowPass) {
        shadowPass->Cleanup();
        shadowPass.reset();
    }

    if (renderPass) {
        renderPass->Cleanup();
        renderPass.reset();
    }

    if (skyboxPass) {
        skyboxPass->Cleanup();
        skyboxPass.reset();
    }

    CleanupOffScreenResources();
}

void Renderer::CleanupOffScreenResources() {
    if (depthImageView != VK_NULL_HANDLE) vkDestroyImageView(device->GetDevice(), depthImageView, nullptr);
    if (depthImage != VK_NULL_HANDLE) vkDestroyImage(device->GetDevice(), depthImage, nullptr);
    if (depthImageMemory != VK_NULL_HANDLE) vkFreeMemory(device->GetDevice(), depthImageMemory, nullptr);

    if (offScreenImageView != VK_NULL_HANDLE) vkDestroyImageView(device->GetDevice(), offScreenImageView, nullptr);
    if (offScreenImage != VK_NULL_HANDLE) vkDestroyImage(device->GetDevice(), offScreenImage, nullptr);
    if (offScreenImageMemory != VK_NULL_HANDLE) vkFreeMemory(device->GetDevice(), offScreenImageMemory, nullptr);

    if (refractionFramebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(device->GetDevice(), refractionFramebuffer, nullptr);
    if (refractionSampler != VK_NULL_HANDLE) vkDestroySampler(device->GetDevice(), refractionSampler, nullptr);
    if (refractionImageView != VK_NULL_HANDLE) vkDestroyImageView(device->GetDevice(), refractionImageView, nullptr);
    if (refractionImage != VK_NULL_HANDLE) vkDestroyImage(device->GetDevice(), refractionImage, nullptr);
    if (refractionImageMemory != VK_NULL_HANDLE) vkFreeMemory(device->GetDevice(), refractionImageMemory, nullptr);
}
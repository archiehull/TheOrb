#include "Renderer.h"
#include "../vulkan/Vertex.h"
#include <stdexcept>

Renderer::Renderer(VulkanDevice* device, VulkanSwapChain* swapChain)
    : device(device), swapChain(swapChain) {
}

Renderer::~Renderer() {
}

void Renderer::Initialize() {
    CreateRenderPass();
    CreateOffScreenResources();

    renderPass->CreateOffScreenFramebuffer(
        offScreenImageView,
        swapChain->GetExtent()
    );

    CreateUniformBuffers();
    CreateDescriptorSets();

    CreatePipeline();
    CreateCommandBuffer();
    CreateSyncObjects();
}

void Renderer::CreateUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i] = std::make_unique<VulkanBuffer>(
            device->GetDevice(),
            device->GetPhysicalDevice()
        );

        uniformBuffers[i]->CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        // Keep mapped for updates
        vkMapMemory(
            device->GetDevice(),
            uniformBuffers[i]->GetBufferMemory(),
            0,
            bufferSize,
            0,
            &uniformBuffersMapped[i]
        );
    }
}

void Renderer::CreateDescriptorSets() {
    descriptorSet = std::make_unique<VulkanDescriptorSet>(device->GetDevice());

    descriptorSet->CreateDescriptorSetLayout();
    descriptorSet->CreateDescriptorPool(MAX_FRAMES_IN_FLIGHT);

    // Get VkBuffer handles from VulkanBuffer wrappers
    std::vector<VkBuffer> buffers;
    for (const auto& uniformBuffer : uniformBuffers) {
        buffers.push_back(uniformBuffer->GetBuffer());
    }

    descriptorSet->CreateDescriptorSets(buffers, sizeof(UniformBufferObject));
}

void Renderer::UpdateUniformBuffer(uint32_t currentFrame, const UniformBufferObject& ubo) {
    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
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
    pipelineConfig.descriptorSetLayout = descriptorSet->GetLayout();

    graphicsPipeline = std::make_unique<GraphicsPipeline>(
        device->GetDevice(),
        pipelineConfig
    );
    graphicsPipeline->Create();
}

void Renderer::CreateOffScreenResources() {
    createImage(
        swapChain->GetExtent().width,
        swapChain->GetExtent().height,
        swapChain->GetImageFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        offScreenImage,
        offScreenImageMemory
    );

    offScreenImageView = createImageView(
        offScreenImage,
        swapChain->GetImageFormat(),
        VK_IMAGE_ASPECT_COLOR_BIT
    );
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
    syncObjects->CreateSyncObjects(swapChain->GetImages().size());
}

void Renderer::DrawFrame(Scene& scene, uint32_t currentFrame) {
    // Wait for this frame's fence
    VkFence fence = syncObjects->GetInFlightFence(currentFrame);
    vkWaitForFences(device->GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

    // Acquire next image - use a temporary semaphore indexed by currentFrame for acquisition
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device->GetDevice(),
        swapChain->GetSwapChain(),
        UINT64_MAX,
        syncObjects->GetImageAvailableSemaphore(currentFrame % swapChain->GetImages().size()),
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
    RecordCommandBuffer(cmd, imageIndex, currentFrame, scene);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { syncObjects->GetImageAvailableSemaphore(currentFrame % swapChain->GetImages().size()) };
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

void Renderer::RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, uint32_t currentFrame, Scene& scene) {
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // Render scene to off-screen
    RenderScene(cmd, currentFrame, scene);

    // Copy to swap chain
    CopyOffScreenToSwapChain(cmd, imageIndex);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Renderer::RenderScene(VkCommandBuffer cmd, uint32_t currentFrame, Scene& scene) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass->GetRenderPass();
    renderPassInfo.framebuffer = renderPass->GetOffScreenFramebuffer();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChain->GetExtent();

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetPipeline());

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline->GetLayout(),
        0,
        1,
        &descriptorSet->GetDescriptorSets()[currentFrame],
        0,
        nullptr
    );

    // Set dynamic states
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->GetExtent().width);
    viewport.height = static_cast<float>(swapChain->GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChain->GetExtent();
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdSetLineWidth(cmd, 1.0f);

    // Draw all objects in the scene
    for (const auto& obj : scene.GetObjects()) {
        if (obj && obj->visible && obj->geometry) {
            obj->geometry->Bind(cmd);
            obj->geometry->Draw(cmd);
        }
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

    if (renderPass) {
        renderPass->Cleanup();
        renderPass.reset();
    }

    CleanupOffScreenResources();
}

void Renderer::CleanupOffScreenResources() {
    if (offScreenImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device->GetDevice(), offScreenImageView, nullptr);
        offScreenImageView = VK_NULL_HANDLE;
    }
    if (offScreenImage != VK_NULL_HANDLE) {
        vkDestroyImage(device->GetDevice(), offScreenImage, nullptr);
        offScreenImage = VK_NULL_HANDLE;
    }
    if (offScreenImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device->GetDevice(), offScreenImageMemory, nullptr);
        offScreenImageMemory = VK_NULL_HANDLE;
    }
}

void Renderer::createImage(uint32_t width, uint32_t height, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device->GetDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device->GetDevice(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device->GetDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device->GetDevice(), image, imageMemory, 0);
}

VkImageView Renderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device->GetPhysicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}
#include "Application.h"
#include <stdexcept>

Application::Application() {
    window = std::make_unique<Window>(800, 600, "Vulkan");
    vulkanContext = std::make_unique<VulkanContext>();
}

Application::~Application() {
}

void Application::Run() {
    InitVulkan();
    MainLoop();
    Cleanup();
}

void Application::InitVulkan() {
    vulkanContext->CreateInstance();
    vulkanContext->SetupDebugMessenger();
    vulkanContext->CreateSurface(window->GetGLFWWindow());

    vulkanDevice = std::make_unique<VulkanDevice>(vulkanContext->GetInstance(), vulkanContext->GetSurface());
    vulkanDevice->PickPhysicalDevice();
    vulkanDevice->CreateLogicalDevice();

    vulkanSwapChain = std::make_unique<VulkanSwapChain>(
        vulkanDevice->GetDevice(),
        vulkanDevice->GetPhysicalDevice(),
        vulkanContext->GetSurface(),
        window->GetGLFWWindow()
    );
    vulkanSwapChain->Create(vulkanDevice->GetQueueFamilies());
    vulkanSwapChain->CreateImageViews();

    // Create graphics pipeline with off-screen rendering support
    vulkanPipeline = std::make_unique<VulkanPipeline>(
        vulkanDevice->GetDevice(),
        vulkanDevice->GetPhysicalDevice(),
        vulkanSwapChain->GetExtent(),
        vulkanSwapChain->GetImageFormat()
    );

    // Pass true to enable off-screen rendering
    vulkanPipeline->CreateRenderPass(true);
    vulkanPipeline->CreateGraphicsPipeline("src/shaders/vert.spv", "src/shaders/frag.spv");

    // Create off-screen resources
    vulkanPipeline->CreateOffScreenResources();

    // Create command buffers
    vulkanCommandBuffer = std::make_unique<VulkanCommandBuffer>(
        vulkanDevice->GetDevice(),
        vulkanDevice->GetPhysicalDevice()
    );
    vulkanCommandBuffer->CreateCommandPool(vulkanDevice->GetQueueFamilies().graphicsFamily.value());
    vulkanCommandBuffer->CreateCommandBuffers(MAX_FRAMES_IN_FLIGHT);

    // Create synchronization objects with swap chain image count
    vulkanSyncObjects = std::make_unique<VulkanSyncObjects>(
        vulkanDevice->GetDevice(),
        MAX_FRAMES_IN_FLIGHT
    );
    vulkanSyncObjects->CreateSyncObjects(vulkanSwapChain->GetImages().size());
}

void Application::MainLoop() {
    while (!window->ShouldClose()) {
        window->PollEvents();
        DrawFrame();
    }

    vkDeviceWaitIdle(vulkanDevice->GetDevice());
}

void Application::DrawFrame() {
    // Wait for the previous frame to finish
    VkFence fence = vulkanSyncObjects->GetInFlightFence(currentFrame);
    vkWaitForFences(vulkanDevice->GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

    // Acquire swap chain image for presentation
    uint32_t imageIndex;
    VkSemaphore imageAvailableSemaphore = vulkanSyncObjects->GetImageAvailableSemaphore(currentFrame);
    VkResult result = vkAcquireNextImageKHR(
        vulkanDevice->GetDevice(),
        vulkanSwapChain->GetSwapChain(),
        UINT64_MAX,
        imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Swap chain needs to be recreated
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Reset fence only after we know we're submitting work
    vkResetFences(vulkanDevice->GetDevice(), 1, &fence);

    // Record command buffer
    VkCommandBuffer commandBuffer = vulkanCommandBuffer->GetCommandBuffer(currentFrame);
    vkResetCommandBuffer(commandBuffer, 0);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // Step 1: Render to off-screen framebuffer
    RenderOffScreen(commandBuffer);

    // Step 2: Copy off-screen image to swap chain image
    CopyOffScreenToSwapChain(commandBuffer, imageIndex);

    // End command buffer
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Use per-image semaphore
    VkSemaphore signalSemaphores[] = { vulkanSyncObjects->GetRenderFinishedSemaphore(imageIndex) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vulkanDevice->GetGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Present the image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { vulkanSwapChain->GetSwapChain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(vulkanDevice->GetPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swap chain needs to be recreated
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::RenderOffScreen(VkCommandBuffer commandBuffer) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkanPipeline->GetRenderPass();
    renderPassInfo.framebuffer = vulkanPipeline->GetOffScreenFramebuffer();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vulkanSwapChain->GetExtent();

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetPipeline());

    // Set dynamic viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vulkanSwapChain->GetExtent().width);
    viewport.height = static_cast<float>(vulkanSwapChain->GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Set dynamic scissor
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = vulkanSwapChain->GetExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Set dynamic line width
    vkCmdSetLineWidth(commandBuffer, 1.0f);

    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void Application::CopyOffScreenToSwapChain(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkImage offScreenImage = vulkanPipeline->GetOffScreenImage();
    VkImage swapChainImage = vulkanSwapChain->GetImages()[imageIndex];
    VkExtent2D extent = vulkanSwapChain->GetExtent();

    // Transition swap chain image from UNDEFINED to TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier1{};
    barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier1.image = swapChainImage;
    barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier1.subresourceRange.baseMipLevel = 0;
    barrier1.subresourceRange.levelCount = 1;
    barrier1.subresourceRange.baseArrayLayer = 0;
    barrier1.subresourceRange.layerCount = 1;
    barrier1.srcAccessMask = 0;
    barrier1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier1
    );

    // Copy image
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset = { 0, 0, 0 };

    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset = { 0, 0, 0 };

    copyRegion.extent = { extent.width, extent.height, 1 };

    vkCmdCopyImage(
        commandBuffer,
        offScreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapChainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copyRegion
    );

    // Transition swap chain image from TRANSFER_DST_OPTIMAL to PRESENT_SRC_KHR
    VkImageMemoryBarrier barrier2{};
    barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier2.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.image = swapChainImage;
    barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier2.subresourceRange.baseMipLevel = 0;
    barrier2.subresourceRange.levelCount = 1;
    barrier2.subresourceRange.baseArrayLayer = 0;
    barrier2.subresourceRange.layerCount = 1;
    barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier2.dstAccessMask = 0;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier2
    );
}

void Application::Cleanup() {
    if (vulkanSyncObjects) {
        vulkanSyncObjects->Cleanup();
    }

    if (vulkanCommandBuffer) {
        vulkanCommandBuffer->Cleanup();
    }

    if (vulkanPipeline) {
        vulkanPipeline->Cleanup();
    }

    if (vulkanSwapChain) {
        vulkanSwapChain->Cleanup();
    }

    if (vulkanDevice) {
        vulkanDevice->Cleanup();
    }

    if (vulkanContext) {
        vulkanContext->Cleanup();
    }
}
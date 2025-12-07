#include "Application.h"

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

    vulkanPipeline = std::make_unique<VulkanPipeline>(
        vulkanDevice->GetDevice(),
        vulkanSwapChain->GetExtent(),
        vulkanSwapChain->GetImageFormat()
	);  
	vulkanPipeline->CreateRenderPass();
    vulkanPipeline->CreateGraphicsPipeline("src/shaders/vert.spv", "src/shaders/frag.spv");
}

void Application::MainLoop() {
    while (!window->ShouldClose()) {
        window->PollEvents();
    }
}

void Application::Cleanup() {
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
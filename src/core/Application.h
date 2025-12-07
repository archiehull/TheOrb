#pragma once

#include "Window.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanDevice.h"
#include "vulkan/VulkanSwapChain.h"
#include "vulkan/VulkanCommandBuffer.h"
#include "vulkan/VulkanSyncObjects.h"
#include "vulkan/VulkanRenderPass.h"

#include "geometry/GeometryGenerator.h"
#include "geometry/Geometry.h"

#include "rendering/Renderer.h"
#include "rendering/Scene.h"
#include "rendering/CameraController.h"

#include "pipelines/GraphicsPipeline.h"

#include <chrono>
#include <memory>
#include <stdexcept>

class Application {
public:
    Application();
    ~Application();

    void Run();

private:
    void InitVulkan();
    void SetupScene();
    void MainLoop();
    void Cleanup();
    // Input handling
    void ProcessInput();
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);


    std::unique_ptr<Window> window;
    std::unique_ptr<VulkanContext> vulkanContext;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<VulkanSwapChain> vulkanSwapChain;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<CameraController> cameraController;


    uint32_t currentFrame = 0;
    bool framebufferResized = false;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    // Timing
    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
    float deltaTime = 0.0f;
};
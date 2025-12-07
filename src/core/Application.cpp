#include "Application.h"
#include <iostream>


Application::Application() {
    window = std::make_unique<Window>(800, 600, "TheOrb");
}

Application::~Application() {
}

void Application::Run() {
    InitVulkan();
    SetupScene();
    MainLoop();
    Cleanup();
}

void Application::InitVulkan() {
    // Create Vulkan infrastructure
    vulkanContext = std::make_unique<VulkanContext>();
    vulkanContext->CreateInstance();
    vulkanContext->SetupDebugMessenger();
    vulkanContext->CreateSurface(window->GetGLFWWindow());

    vulkanDevice = std::make_unique<VulkanDevice>(
        vulkanContext->GetInstance(),
        vulkanContext->GetSurface()
    );
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

    // Create renderer
    renderer = std::make_unique<Renderer>(
        vulkanDevice.get(),
        vulkanSwapChain.get()
    );
    renderer->Initialize();

    // Create scene
    scene = std::make_unique<Scene>(
        vulkanDevice->GetDevice(),
        vulkanDevice->GetPhysicalDevice()
    );

	// Create camera controller
    cameraController = std::make_unique<CameraController>();
}

void Application::SetupScene() {

    //scene->AddTriangle();

    // scene->AddCircle(64, 0.3f);
    // scene->AddQuad();
     //scene->AddCube();
     scene->AddGrid(10, 10, 0.05f);
}

void Application::MainLoop() {
    while (!window->ShouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        window->PollEvents();
        ProcessInput();

        // Update camera
        cameraController->Update(deltaTime);

        // Update uniform buffer with active camera
        Camera* activeCamera = cameraController->GetActiveCamera();
        UniformBufferObject ubo{};
        ubo.model = glm::mat4(1.0f);
        ubo.view = activeCamera->GetViewMatrix();
        ubo.proj = activeCamera->GetProjectionMatrix(
            vulkanSwapChain->GetExtent().width / (float)vulkanSwapChain->GetExtent().height
        );

        renderer->UpdateUniformBuffer(currentFrame, ubo);
        renderer->DrawFrame(*scene, currentFrame);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    renderer->WaitIdle();
}

void Application::ProcessInput() {
    // ESC to close
    if (glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window->GetGLFWWindow(), true);
    }
}

void Application::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS) {
        // Camera switching
        if (key == GLFW_KEY_F1) {
            app->cameraController->SwitchCamera(CameraType::BIRDS_EYE);
            std::cout << "Switched to Birds Eye Camera (F1)" << std::endl;
        }
        else if (key == GLFW_KEY_F2) {
            app->cameraController->SwitchCamera(CameraType::FREE_ROAM);
            std::cout << "Switched to Free Roam Camera (F2)" << std::endl;
        }
        else if (key == GLFW_KEY_F3) {
            app->cameraController->SwitchCamera(CameraType::ORBIT);
            std::cout << "Switched to Orbit Camera (F3)" << std::endl;
        }

        // Forward key press to camera controller
        app->cameraController->OnKeyPress(key, true);
    }
    else if (action == GLFW_RELEASE) {
        app->cameraController->OnKeyRelease(key);
    }
}

void Application::Cleanup() {
    if (scene) {
        scene->Cleanup();
        scene.reset();
    }

    if (renderer) {
        renderer->Cleanup();
        renderer.reset();
    }

    if (vulkanSwapChain) {
        vulkanSwapChain->Cleanup();
        vulkanSwapChain.reset();
    }

    if (vulkanDevice) {
        vulkanDevice->Cleanup();
        vulkanDevice.reset();
    }

    if (vulkanContext) {
        vulkanContext->Cleanup();
        vulkanContext.reset();
    }

}
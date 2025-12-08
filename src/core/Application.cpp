#include "Application.h"
#include <iostream>


Application::Application() {
    window = std::make_unique<Window>(800, 600, "TheOrb");

    glfwSetWindowUserPointer(window->GetGLFWWindow(), this);
    glfwSetKeyCallback(window->GetGLFWWindow(), KeyCallback);
    glfwSetFramebufferSizeCallback(window->GetGLFWWindow(), FramebufferResizeCallback);
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
    // Grid parameters
    const int rows = 10;
    const int cols = 10;
    const float cellSize = 0.5f;

    // Add a centered grid at origin (XZ plane, y=0)
    scene->AddGrid(rows, cols, cellSize, glm::vec3(0.0f, 0.0f, 0.0f), "textures/desert.jpg");

    // Compute grid extents (GeometryGenerator centers the grid)
    const float width = cols * cellSize;
    const float depth = rows * cellSize;
    const float startX = -width * 0.5f;
    const float startZ = -depth * 0.5f;

    // Cube sits on grid: cube half-height = 0.5 -> center y = 0.5
    const float cubeY = 0.5f;

    // Four corners in XZ plane (bottom-left, bottom-right, top-right, top-left)
    scene->AddCube(glm::vec3(startX, cubeY, startZ));                    // bottom-left
    scene->AddCube(glm::vec3(startX + width, cubeY, startZ));            // bottom-right
    scene->AddCube(glm::vec3(startX + width, cubeY, startZ + depth));    // top-right
    scene->AddCube(glm::vec3(startX, cubeY, startZ + depth));            // top-left

    // Floating cube in the center, above the grid
    scene->AddCube(glm::vec3(0.0f, 2.0f, 0.0f));
}

void Application::RecreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window->GetGLFWWindow(), &width, &height);

    // Handle minimization
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window->GetGLFWWindow(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(vulkanDevice->GetDevice());

    // Cleanup old swapchain-dependent resources
    renderer->Cleanup();
    vulkanSwapChain->Cleanup();

    // Recreate swapchain
    vulkanSwapChain->Create(vulkanDevice->GetQueueFamilies());
    vulkanSwapChain->CreateImageViews();

    // Recreate renderer resources
    renderer->Initialize();

    framebufferResized = false;
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

        // Get camera matrices
        Camera* activeCamera = cameraController->GetActiveCamera();
        glm::mat4 viewMatrix = activeCamera->GetViewMatrix();
        glm::mat4 projMatrix = activeCamera->GetProjectionMatrix(
            vulkanSwapChain->GetExtent().width / (float)vulkanSwapChain->GetExtent().height
        );

        // Pass matrices to renderer (UBO update now happens in RenderScene)
        renderer->DrawFrame(*scene, currentFrame, viewMatrix, projMatrix);

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

void Application::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
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
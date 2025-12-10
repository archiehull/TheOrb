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
    scene->AddGrid(10, 10, 0.5f, glm::vec3(0.0f, 0.0f, 0.0f), "textures/desert.jpg");

   	scene->AddSphere(16, 32, 0.5f, glm::vec3(0.0f, 4.0f, 0.0f), "textures/moon.jpg");
	scene->AddLight(glm::vec3(0.0f, 4.0f, 0.0f), glm::vec3(0.5f, 0.0f, 0.0f), 0.5f, 0);

	scene->AddSphere(16, 32, 0.3f, glm::vec3(3.0f, 2.0f, -2.0f), "textures/sun.png");
	scene->AddLight(glm::vec3(3.0f, 2.0f, -2.0f), glm::vec3(0.5f, 0.5f, 1.0f), 1.0f, 0);

	scene->AddModel(glm::vec3(2.0f, 0.0f, -1.0f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(0.07f), "models/DeadTree.obj", "textures/bark.jpg");
    scene->AddModel(glm::vec3(-2.0f, 0.0f, 1.0f), glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.05f), "models/DeadTree.obj", "textures/bark.jpg");
    scene->AddModel(glm::vec3(-1.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.08f), "models/DeadTree.obj", "textures/bark.jpg");
    scene->AddModel(glm::vec3(1.0f, 0.0f, -2.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(0.09f), "models/DeadTree.obj", "textures/bark.jpg");

	scene->AddModel(glm::vec3(0.0f, -0.1f, 0.0f), glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(0.006f), "models/cactus.obj", "textures/cactus.jpg");
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

        if (framebufferResized) {
            RecreateSwapChain();
        }

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
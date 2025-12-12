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

    lastFrameTime = std::chrono::high_resolution_clock::now();


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
    float deltaY = -50.0f;

    scene->AddGrid("GroundGrid", 10, 10, 20.0f, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), "textures/desert.jpg");

    float orbitRadius = 200.0f;
    float startSpeed = 0.5f;
    dayNightSpeed = startSpeed;

    scene->AddSphere("Sun", 16, 32, 20.0f, glm::vec3(0.0f), "textures/sun.png");
    scene->AddLight("Sun", glm::vec3(0.0f), glm::vec3(1.0f, 0.9f, 0.8f), 1.0f, 0);
    scene->SetObjectCastsShadow("Sun", false);

    scene->SetObjectOrbit("Sun", glm::vec3(0.0f, 0.0f, 0.0f), orbitRadius, startSpeed, glm::vec3(0.0f, 0.0f, 1.0f), 0.0f);
    scene->SetLightOrbit("Sun", glm::vec3(0.0f, 0.0f, 0.0f), orbitRadius, startSpeed, glm::vec3(0.0f, 0.0f, 1.0f), 0.0f);

    scene->AddSphere("Moon", 16, 32, 12.0f, glm::vec3(0.0f), "textures/moon.jpg");
    scene->AddLight("Moon", glm::vec3(0.0f), glm::vec3(0.1f, 0.1f, 0.3f), 1.5f, 0);
    scene->SetObjectCastsShadow("Moon", false);


    scene->SetObjectOrbit("Moon", glm::vec3(0.0f, 0.0f, 0.0f), orbitRadius, startSpeed, glm::vec3(0.0f, 0.0f, 1.0f), glm::pi<float>());
    scene->SetLightOrbit("Moon", glm::vec3(0.0f, 0.0f, 0.0f), orbitRadius, startSpeed, glm::vec3(0.0f, 0.0f, 1.0f), glm::pi<float>());

    scene->AddModel("Tree1", glm::vec3(80.0f, 0.0f + deltaY, -40.0f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(2.8f), "models/DeadTree.obj", "textures/bark.jpg");
    scene->AddModel("Tree2", glm::vec3(-80.0f, 0.0f + deltaY, 40.0f), glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(2.0f), "models/DeadTree.obj", "textures/bark.jpg");
    scene->AddModel("Tree3", glm::vec3(-40.0f, 0.0f + deltaY, 80.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(3.2f), "models/DeadTree.obj", "textures/bark.jpg");
    scene->AddModel("Tree4", glm::vec3(40.0f, 0.0f + deltaY, -80.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(3.6f), "models/DeadTree.obj", "textures/bark.jpg");

    scene->AddModel("Cactus", glm::vec3(0.0f, -4.0f + deltaY, 0.0f), glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(0.24f), "models/cactus.obj", "textures/cactus.jpg");


    scene->AddSphere("CrystalBall", 32, 64, 150.0f, glm::vec3(0.0f, 0.0f, 0.0f), "");

    scene->SetObjectShadingMode("CrystalBall", 3);

    scene->SetObjectCastsShadow("CrystalBall", false);

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

        cameraController->Update(deltaTime);
        scene->Update(deltaTime);

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

    bool speedChanged = false;
    float speedChangeRate = 2.0f; // How fast the speed adjusts

    if (glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        dayNightSpeed += speedChangeRate * deltaTime;
        speedChanged = true;
    }
    if (glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        dayNightSpeed -= speedChangeRate * deltaTime;
        speedChanged = true;
    }

    if (speedChanged) {
        // Apply new speed to both Sun and Moon (Mesh + Light)
        scene->SetOrbitSpeed("Sun", dayNightSpeed);
        scene->SetOrbitSpeed("Moon", dayNightSpeed);

        //std::cout << "Orbit Speed: " << dayNightSpeed << std::endl; // Optional debug
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
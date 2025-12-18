#include "Application.h"
#include "../rendering/ParticleLibrary.h"
#include <iostream>


Application::Application()
    : window(std::make_unique<Window>(800, 600, "TheOrb"))
{
    glfwSetWindowUserPointer(window->GetGLFWWindow(), this);
    glfwSetKeyCallback(window->GetGLFWWindow(), KeyCallback);
    glfwSetFramebufferSizeCallback(window->GetGLFWWindow(), FramebufferResizeCallback);
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

    renderer->SetupSceneParticles(*scene);

    // Create camera controller
    cameraController = std::make_unique<CameraController>();
}

static const char* SUN_NAME = "Sun";
static const char* MOON_NAME = "Moon";

void Application::SetupScene() {
    const float orbitRadius = 275.0f;
    const float startSpeed = 0.1f;
    dayNightSpeed = startSpeed;

    const float deltaY = -75.0f;
    const float orbRadius = 150.0f;
    const float terrainHeightScale = 3.5f;
    const float terrainNoiseFreq = 0.02f;

    const float adjustedRadius = scene->RadiusAdjustment(orbRadius, deltaY);

    scene->AddTerrain("GroundGrid", adjustedRadius, 512, 512, 3.5f, 0.02f, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), "textures/desert2.jpg");
    scene->AddPedestal("BasePedestal", adjustedRadius, orbRadius * 2.3, 100.0f, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), "textures/mahogany.jpg");
    scene->SetObjectCastsShadow("BasePedestal", false);
    scene->SetObjectLayerMask("BasePedestal", SceneLayers::OUTSIDE);

    // High frequency cacti (small)
    scene->RegisterProceduralObject("models/cactus.obj", "textures/cactus.jpg", 7.0f, glm::vec3(0.01f), glm::vec3(0.02f), glm::vec3(-90.0f, 0.0f, 0.0f));
    // Medium frequency dead trees
    scene->RegisterProceduralObject("models/DeadTree.obj", "textures/bark.jpg", 5.0f, glm::vec3(0.1f), glm::vec3(0.2f));
    // Low frequency large trees
    scene->RegisterProceduralObject("models/DeadTree.obj", "textures/bark.jpg", 4.0f, glm::vec3(0.25f), glm::vec3(0.35f));
    scene->GenerateProceduralObjects(50, orbRadius - 20, deltaY, terrainHeightScale, terrainNoiseFreq);

    // sun must be called first
    scene->AddSphere(SUN_NAME, 16, 32, 5.0f, glm::vec3(0.0f), "textures/sun.png");
    scene->AddLight(SUN_NAME, glm::vec3(0.0f), glm::vec3(1.0f, 0.9f, 0.8f), 1.0f, 0);
    scene->SetObjectCastsShadow(SUN_NAME, false);
    scene->SetObjectOrbit(SUN_NAME, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), orbitRadius, startSpeed, glm::vec3(0.0f, 0.0f, 1.0f), 0.0f);
    scene->SetLightOrbit(SUN_NAME, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), orbitRadius, startSpeed, glm::vec3(0.0f, 0.0f, 1.0f), 0.0f);
    scene->SetObjectLayerMask(SUN_NAME, SceneLayers::ALL);
    scene->SetLightLayerMask(SUN_NAME, SceneLayers::ALL);

    scene->AddSphere(MOON_NAME, 16, 32, 2.0f, glm::vec3(0.0f), "textures/moon.jpg");
    scene->AddLight(MOON_NAME, glm::vec3(0.0f), glm::vec3(0.1f, 0.1f, 0.3f), 1.5f, 0);
    scene->SetObjectCastsShadow(MOON_NAME, false);
    scene->SetObjectOrbit(MOON_NAME, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), orbitRadius, startSpeed, glm::vec3(0.0f, 0.0f, 1.0f), glm::pi<float>());
    scene->SetLightOrbit(MOON_NAME, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), orbitRadius, startSpeed, glm::vec3(0.0f, 0.0f, 1.0f), glm::pi<float>());
    scene->SetObjectLayerMask(MOON_NAME, SceneLayers::ALL);
    scene->SetLightLayerMask(MOON_NAME, SceneLayers::ALL);

    scene->AddSphere("PedestalLightSphere", 16, 32, 5.0f, glm::vec3(200.0f, 0.0f, 200.0f));
    scene->AddLight("PedestalLight", glm::vec3(200.0f, 0.0f, 200.0f), glm::vec3(1.0f, 0.5f, 0.2f), 5.0f, 0);
    scene->SetLightLayerMask("PedestalLight", SceneLayers::OUTSIDE);
    scene->SetObjectLayerMask("PedestalLightSphere", SceneLayers::OUTSIDE);

    scene->AddSphere("CrystalBall", 32, 64, orbRadius, glm::vec3(0.0f, 0.0f, 0.0f), "");
    scene->SetObjectShadingMode("CrystalBall", 3);
    scene->SetObjectCastsShadow("CrystalBall", false);

    scene->AddSphere("FogShell", 32, 64, orbRadius + 1, glm::vec3(0.0f, 0.0f, 0.0f), "");
    scene->SetObjectShadingMode("FogShell", 4);
    scene->SetObjectCastsShadow("FogShell", false);
    scene->SetObjectLayerMask("FogShell", 0x1 | 0x2);

    scene->AddFire(glm::vec3(0.0f, 0.5f + deltaY, 0.0f), 1.0f, true);   // Fire + Smoke
    scene->AddFire(glm::vec3(-25.0f, 0.5f + deltaY, 0.0f), 1.0f, true);

    // Add Snow
    scene->AddSnow();
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
        const auto currentTime = std::chrono::high_resolution_clock::now();
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
        Camera* const activeCamera = cameraController->GetActiveCamera();
        const glm::mat4 viewMatrix = activeCamera->GetViewMatrix();
        // Rule ID: CODSTA-CPP.11 - C++ style cast
        const glm::mat4 projMatrix = activeCamera->GetProjectionMatrix(
            vulkanSwapChain->GetExtent().width / static_cast<float>(vulkanSwapChain->GetExtent().height)
        );

        int currentViewMask = SceneLayers::ALL; // Default

        // Check distance to center (0,0,0)
        const float dist = glm::length(activeCamera->GetPosition());
        const float ballRadius = 150.0f; // Matches your setup

        if (dist < ballRadius) {
            // We are INSIDE: Draw Terrain + Sun/Moon
            currentViewMask = SceneLayers::INSIDE;
        }
        else {
            // We are OUTSIDE: Draw Room/Pedestal + Crystal Ball + Sun/Moon
            currentViewMask = SceneLayers::ALL;
        }

        // Pass 'currentViewMask' to DrawFrame
        renderer->DrawFrame(*scene, currentFrame, viewMatrix, projMatrix, currentViewMask);

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
    const float speedChangeRate = 1.2f; // How fast the speed adjusts

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
        scene->SetOrbitSpeed(SUN_NAME, dayNightSpeed);
        scene->SetOrbitSpeed(MOON_NAME, dayNightSpeed);

        //std::cout << "Orbit Speed: " << dayNightSpeed << std::endl; // Optional debug
    }
}

// Rule ID: CODSTA.45 - Renamed parameter 'window' to 'glfwWindow'
void Application::KeyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {
    auto* const app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(glfwWindow));

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

void Application::FramebufferResizeCallback(GLFWwindow* glfwWindow, int width, int height) {
    // Rule ID: CODSTA-CPP.53
    auto* const app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(glfwWindow));
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
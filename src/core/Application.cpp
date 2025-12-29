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
    const float dayDuration = 60.0f;
    const float baseOrbitSpeed = glm::two_pi<float>() / dayDuration;

    const float orbitRadius = 275.0f;
    const float deltaY = -75.0f;
    const float orbRadius = 150.0f;
    const float terrainHeightScale = 3.5f;
    const float terrainNoiseFreq = 0.02f;

    const float adjustedRadius = scene->RadiusAdjustment(orbRadius, deltaY);

    scene->AddTerrain("GroundGrid", adjustedRadius, 512, 512, 3.5f, 0.02f, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), "textures/desert2.jpg");

    scene->AddPedestal("BasePedestal", adjustedRadius, orbRadius * 2.3, 100.0f, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), "textures/mahogany.jpg");
    scene->SetObjectCastsShadow("BasePedestal", false);
    scene->SetObjectLayerMask("BasePedestal", SceneLayers::OUTSIDE);
    scene->SetObjectCollision("BasePedestal", false);


	// High frequency cacti
    scene->RegisterProceduralObject("models/cactus.obj", "textures/cactus.jpg", 7.0f, glm::vec3(0.01f), glm::vec3(0.02f), glm::vec3(-90.0f, 0.0f, 0.0f), true);
	// Medium frequency dead trees
    scene->RegisterProceduralObject("models/DeadTree.obj", "textures/bark.jpg", 5.0f, glm::vec3(0.1f), glm::vec3(0.2f), glm::vec3(0.0f), true);
	// Low frequency dead trees (larger)
    scene->RegisterProceduralObject("models/DeadTree.obj", "textures/bark.jpg", 4.0f, glm::vec3(0.25f), glm::vec3(0.35f), glm::vec3(0.0f), true);
    scene->GenerateProceduralObjects(50, orbRadius - 20, deltaY, terrainHeightScale, terrainNoiseFreq);

    // sun must be called first
    scene->AddSphere(SUN_NAME, 16, 32, 5.0f, glm::vec3(0.0f), "textures/sun.png");
    scene->AddLight(SUN_NAME, glm::vec3(0.0f), glm::vec3(1.0f, 0.9f, 0.8f), 1.0f, 0);
    scene->SetObjectCastsShadow(SUN_NAME, false);
    scene->SetObjectOrbit(SUN_NAME, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), orbitRadius, baseOrbitSpeed, glm::vec3(0.0f, 0.0f, 1.0f), 0.0f);
    scene->SetLightOrbit(SUN_NAME, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), orbitRadius, baseOrbitSpeed, glm::vec3(0.0f, 0.0f, 1.0f), 0.0f);
    scene->SetObjectLayerMask(SUN_NAME, SceneLayers::ALL);
    scene->SetLightLayerMask(SUN_NAME, SceneLayers::ALL);
    scene->SetObjectCollision(SUN_NAME, false);

    scene->AddSphere(MOON_NAME, 16, 32, 2.0f, glm::vec3(0.0f), "textures/moon.jpg");
    scene->AddLight(MOON_NAME, glm::vec3(0.0f), glm::vec3(0.1f, 0.1f, 0.3f), 1.5f, 0);
    scene->SetObjectCastsShadow(MOON_NAME, false);
    scene->SetObjectOrbit(MOON_NAME, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), orbitRadius, baseOrbitSpeed, glm::vec3(0.0f, 0.0f, 1.0f), glm::pi<float>());
    scene->SetLightOrbit(MOON_NAME, glm::vec3(0.0f, 0.0f + deltaY, 0.0f), orbitRadius, baseOrbitSpeed, glm::vec3(0.0f, 0.0f, 1.0f), glm::pi<float>());
    scene->SetObjectLayerMask(MOON_NAME, SceneLayers::ALL);
    scene->SetLightLayerMask(MOON_NAME, SceneLayers::ALL);
    scene->SetObjectCollision(MOON_NAME, false);

    scene->AddSphere("PedestalLightSphere", 16, 32, 5.0f, glm::vec3(200.0f, 0.0f, 200.0f));
    scene->AddLight("PedestalLight", glm::vec3(200.0f, 0.0f, 200.0f), glm::vec3(1.0f, 0.5f, 0.2f), 5.0f, 0);
    scene->SetLightLayerMask("PedestalLight", SceneLayers::OUTSIDE);
    scene->SetObjectLayerMask("PedestalLightSphere", SceneLayers::OUTSIDE);
    scene->SetObjectCollision("PedestalLightSphere", false);

    scene->AddSphere("CrystalBall", 32, 64, orbRadius, glm::vec3(0.0f, 0.0f, 0.0f), "");
    scene->SetObjectShadingMode("CrystalBall", 3);
    scene->SetObjectCastsShadow("CrystalBall", false);
    scene->SetObjectCollision("CrystalBall", false);

    scene->AddSphere("FogShell", 32, 64, orbRadius + 1, glm::vec3(0.0f, 0.0f, 0.0f), "");
    scene->SetObjectShadingMode("FogShell", 4);
    scene->SetObjectCastsShadow("FogShell", false);
    scene->SetObjectLayerMask("FogShell", 0x1 | 0x2);
    scene->SetObjectCollision("FogShell", false);

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

    renderer->SetupSceneParticles(*scene);

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

        scene->Update(deltaTime * timeScale);
        cameraController->Update(deltaTime, *scene);


        // Get camera matrices
        Camera* const activeCamera = cameraController->GetActiveCamera();
        const glm::mat4 viewMatrix = activeCamera->GetViewMatrix();
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

    const float scaleChangeRate = 2.0f;

    if (glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        timeScale += scaleChangeRate * deltaTime;
        //std::cout << "Time Scale: " << timeScale << "x" << std::endl;
    }
    if (glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        timeScale -= scaleChangeRate * deltaTime;
        if (timeScale < 0.1f) timeScale = 0.1f; // Minimum speed
        //std::cout << "Time Scale: " << timeScale << "x" << std::endl;
    }

    if (glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_R) == GLFW_PRESS) {
        timeScale = 1.0f; // Reset speed
        scene->ResetEnvironment();
        //std::cout << "Environment Reset." << std::endl;
    }
}

void Application::KeyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {
    auto* const app = static_cast<Application*>(glfwGetWindowUserPointer(glfwWindow));

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
    auto* const app = static_cast<Application*>(glfwGetWindowUserPointer(glfwWindow));
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
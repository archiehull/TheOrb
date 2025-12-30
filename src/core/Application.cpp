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

    // CHANGE: Initialize with Static Camera (F1)
    cameraController->SwitchCamera(CameraType::BIRDS_EYE, *scene);

    MainLoop();
    Cleanup();
}

void Application::InitVulkan() {
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

    renderer = std::make_unique<Renderer>(
        vulkanDevice.get(),
        vulkanSwapChain.get()
    );
    renderer->Initialize();

    scene = std::make_unique<Scene>(
        vulkanDevice->GetDevice(),
        vulkanDevice->GetPhysicalDevice()
    );

    renderer->SetupSceneParticles(*scene);

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

    scene->AddSnow();
}

void Application::RecreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window->GetGLFWWindow(), &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window->GetGLFWWindow(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(vulkanDevice->GetDevice());

    renderer->Cleanup();
    vulkanSwapChain->Cleanup();

    vulkanSwapChain->Create(vulkanDevice->GetQueueFamilies());
    vulkanSwapChain->CreateImageViews();

    renderer->Initialize();
    renderer->SetupSceneParticles(*scene);

    framebufferResized = false;
}

void Application::MainLoop() {
    while (!window->ShouldClose()) {
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

        Camera* const activeCamera = cameraController->GetActiveCamera();
        const glm::mat4 viewMatrix = activeCamera->GetViewMatrix();
        const glm::mat4 projMatrix = activeCamera->GetProjectionMatrix(
            vulkanSwapChain->GetExtent().width / static_cast<float>(vulkanSwapChain->GetExtent().height)
        );

        int currentViewMask = SceneLayers::ALL;

        const float dist = glm::length(activeCamera->GetPosition());
        const float ballRadius = 150.0f;

        if (dist < ballRadius) {
            currentViewMask = SceneLayers::INSIDE;
        }
        else {
            currentViewMask = SceneLayers::ALL;
        }

        renderer->DrawFrame(*scene, currentFrame, viewMatrix, projMatrix, currentViewMask);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    renderer->WaitIdle();
}

void Application::ProcessInput() {
    if (glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window->GetGLFWWindow(), true);
    }

    const float scaleChangeRate = 2.0f;

    bool shiftPressed = glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    bool ctrlPressed = glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    if (glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_T) == GLFW_PRESS) {
        if (ctrlPressed) {
            timeScale = 1.0f;
        }
        else if (shiftPressed) {
            timeScale += scaleChangeRate * deltaTime;
        }
        else {
            timeScale -= scaleChangeRate * deltaTime;
            if (timeScale < 0.1f) timeScale = 0.1f;
        }
    }

    if (glfwGetKey(window->GetGLFWWindow(), GLFW_KEY_R) == GLFW_PRESS) {
        timeScale = 1.0f;
        scene->ResetEnvironment();
    }
}

void Application::KeyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {
    auto* const app = static_cast<Application*>(glfwGetWindowUserPointer(glfwWindow));

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_F1) {
            app->cameraController->SwitchCamera(CameraType::BIRDS_EYE, *app->scene);
            std::cout << "Switched to Static Camera (F1)" << std::endl;
        }
        else if (key == GLFW_KEY_F2) {
            app->cameraController->SwitchCamera(CameraType::FREE_ROAM, *app->scene);
            std::cout << "Switched to Free Roam Camera (F2)" << std::endl;
        }
        else if (key == GLFW_KEY_F3) {
            app->cameraController->SwitchCamera(CameraType::ORBIT, *app->scene);
            std::cout << "Switched to Cactus Orbit Camera (F3)" << std::endl;
        }
        else if (key == GLFW_KEY_Y) {
            app->scene->ToggleGlobalShadingMode();
        }

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
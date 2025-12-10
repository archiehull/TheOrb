#include "CameraController.h"
#include <GLFW/glfw3.h>

CameraController::CameraController()
    : activeCamera(nullptr)
    , activeCameraType(CameraType::FREE_ROAM)
{
    SetupCameras();
    SwitchCamera(CameraType::FREE_ROAM);
}

CameraController::~CameraController() {
}

void CameraController::SetupCameras() {
    // FREE ROAM Camera - starts at a nice viewing position
    auto freeRoamCam = std::make_unique<Camera>();
    freeRoamCam->SetPosition(glm::vec3(0.0f, 3.0f, 10.0f));
    freeRoamCam->SetTarget(glm::vec3(0.0f, 2.0f, 0.0f));
    freeRoamCam->SetMoveSpeed(2.0f);
    freeRoamCam->SetRotateSpeed(35.0f);
    cameras[CameraType::FREE_ROAM] = std::move(freeRoamCam);

    // BIRDS EYE Camera - looking straight down
    auto birdsEyeCam = std::make_unique<Camera>();
    birdsEyeCam->SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
    birdsEyeCam->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    birdsEyeCam->SetUp(glm::vec3(0.0f, 0.0f, -1.0f));  // Z is up when looking down
    cameras[CameraType::BIRDS_EYE] = std::move(birdsEyeCam);

    // ORBIT Camera - looking at center from side
    auto orbitCam = std::make_unique<Camera>();
    orbitCam->SetPosition(glm::vec3(3.0f, 0.0f, 0.0f));
    orbitCam->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    cameras[CameraType::ORBIT] = std::move(orbitCam);
}

void CameraController::SwitchCamera(CameraType type) {
    if (cameras.find(type) != cameras.end()) {
        activeCameraType = type;
        activeCamera = cameras[type].get();
    }
}

void CameraController::Update(float deltaTime) {
    if (activeCameraType == CameraType::FREE_ROAM) {
        UpdateFreeRoamCamera(deltaTime);
    }
    // Fixed cameras don't need updates
}

void CameraController::UpdateFreeRoamCamera(float deltaTime) {
    if (!activeCamera) return;

    // View rotation with IJKL or CTRL+WASD
    if (keyCtrl) {
        // CTRL+WASD for rotation
        if (keyW) activeCamera->RotatePitch(deltaTime);   // Look up
        if (keyS) activeCamera->RotatePitch(-deltaTime);  // Look down
        if (keyA) activeCamera->RotateYaw(-deltaTime);    // Look left
        if (keyD) activeCamera->RotateYaw(deltaTime);     // Look right
    }
    else {
        // Movement with WASD
        if (keyW) activeCamera->MoveForward(deltaTime);
        if (keyS) activeCamera->MoveBackward(deltaTime);
        if (keyA) activeCamera->MoveLeft(deltaTime);
        if (keyD) activeCamera->MoveRight(deltaTime);
        if (keyQ) activeCamera->MoveDown(deltaTime);
        if (keyE) activeCamera->MoveUp(deltaTime);
        // IJKL for rotation
        if (keyI) activeCamera->RotatePitch(deltaTime);   // Look up
        if (keyK) activeCamera->RotatePitch(-deltaTime);  // Look down
        if (keyJ) activeCamera->RotateYaw(-deltaTime);    // Look left
        if (keyL) activeCamera->RotateYaw(deltaTime);     // Look right
    }
}

void CameraController::OnKeyPress(int key, bool pressed) {
    // Movement keys
    if (key == GLFW_KEY_W) keyW = pressed;
    if (key == GLFW_KEY_A) keyA = pressed;
    if (key == GLFW_KEY_S) keyS = pressed;
    if (key == GLFW_KEY_D) keyD = pressed;
    if (key == GLFW_KEY_Q) keyQ = pressed;
    if (key == GLFW_KEY_E) keyE = pressed;

    // Rotation keys
    if (key == GLFW_KEY_I) keyI = pressed;
    if (key == GLFW_KEY_J) keyJ = pressed;
    if (key == GLFW_KEY_K) keyK = pressed;
    if (key == GLFW_KEY_L) keyL = pressed;

    // Modifier
    if(key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
        keyCtrl = pressed;
    }
}

void CameraController::OnKeyRelease(int key) {
    OnKeyPress(key, false);
}
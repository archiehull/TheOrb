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

    // Compute exclusive (swapped) input mapping so one key set can't trigger both move and rotate.
    // Default (no CTRL): Group A (WASD) = movement, Group B (IJKL + Arrows) = rotation.
    // With CTRL held: groups swap roles (WASD = rotation, IJKL+Arrows = movement).
    bool groupA_forward   = keyW;
    bool groupA_backward  = keyS;
    bool groupA_left      = keyA;
    bool groupA_right     = keyD;
    bool groupB_forward   = keyI || keyUp;
    bool groupB_backward  = keyK || keyDown;
    bool groupB_left      = keyJ || keyLeft;
    bool groupB_right     = keyL || keyRight;

    // Effective movement flags
    bool moveForward  = keyCtrl ? groupB_forward  : groupA_forward;
    bool moveBackward = keyCtrl ? groupB_backward : groupA_backward;
    bool moveLeft     = keyCtrl ? groupB_left     : groupA_left;
    bool moveRight    = keyCtrl ? groupB_right    : groupA_right;

    // Vertical movement (Q/E) remain movement in both modes.
    bool moveDown = keyQ;
    bool moveUp   = keyE;

    // Effective rotation flags (pitch = look up/down, yaw = look left/right)
    bool rotatePitchUp    = keyCtrl ? groupA_forward  : groupB_forward;  // Look up
    bool rotatePitchDown  = keyCtrl ? groupA_backward : groupB_backward; // Look down
    bool rotateYawLeft    = keyCtrl ? groupA_left     : groupB_left;     // Look left
    bool rotateYawRight   = keyCtrl ? groupA_right    : groupB_right;    // Look right

    // Apply movement (exclusive)
    if (moveForward)  activeCamera->MoveForward(deltaTime);
    if (moveBackward) activeCamera->MoveBackward(deltaTime);
    if (moveLeft)     activeCamera->MoveLeft(deltaTime);
    if (moveRight)    activeCamera->MoveRight(deltaTime);
    if (moveDown)     activeCamera->MoveDown(deltaTime);
    if (moveUp)       activeCamera->MoveUp(deltaTime);

    // Apply rotation (exclusive)
    if (rotatePitchUp)   activeCamera->RotatePitch(deltaTime);
    if (rotatePitchDown) activeCamera->RotatePitch(-deltaTime);
    if (rotateYawLeft)   activeCamera->RotateYaw(-deltaTime);
    if (rotateYawRight)  activeCamera->RotateYaw(deltaTime);
}

void CameraController::OnKeyPress(int key, bool pressed) {
    // Movement keys
    if (key == GLFW_KEY_W) keyW = pressed;
    if (key == GLFW_KEY_A) keyA = pressed;
    if (key == GLFW_KEY_S) keyS = pressed;
    if (key == GLFW_KEY_D) keyD = pressed;
    if (key == GLFW_KEY_Q) keyQ = pressed;
    if (key == GLFW_KEY_E) keyE = pressed;

    // Rotation / alternate movement keys (IJKL)
    if (key == GLFW_KEY_I) keyI = pressed;
    if (key == GLFW_KEY_J) keyJ = pressed;
    if (key == GLFW_KEY_K) keyK = pressed;
    if (key == GLFW_KEY_L) keyL = pressed;

    // Arrow keys handled separately so they can participate in swap
    if (key == GLFW_KEY_UP) keyUp = pressed;
    if (key == GLFW_KEY_LEFT) keyLeft = pressed;
    if (key == GLFW_KEY_DOWN) keyDown = pressed;
    if (key == GLFW_KEY_RIGHT) keyRight = pressed;

    // Modifier
    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
        keyCtrl = pressed;
    }
}

void CameraController::OnKeyRelease(int key) {
    OnKeyPress(key, false);
}
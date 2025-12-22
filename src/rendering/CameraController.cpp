#include "CameraController.h"
#include <GLFW/glfw3.h>

CameraController::CameraController()
    : activeCamera(nullptr)
    , activeCameraType(CameraType::FREE_ROAM)
{
    SetupCameras();
    SwitchCamera(CameraType::FREE_ROAM);
}

void CameraController::SetupCameras() {
    // FREE ROAM Camera
    auto freeRoamCam = std::make_unique<Camera>();
    // SCALED UP: Move back and up
    freeRoamCam->SetPosition(glm::vec3(0.0f, 60.0f, 300.0f));
    freeRoamCam->SetTarget(glm::vec3(0.0f, 40.0f, 0.0f));
    freeRoamCam->SetMoveSpeed(50.0f); // Faster default
    freeRoamCam->SetRotateSpeed(35.0f);
    cameras[CameraType::FREE_ROAM] = std::move(freeRoamCam);

    // BIRDS EYE Camera
    auto birdsEyeCam = std::make_unique<Camera>();
    // SCALED UP: High up
    birdsEyeCam->SetPosition(glm::vec3(0.0f, 350.0f, 0.0f));
    birdsEyeCam->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    birdsEyeCam->SetUp(glm::vec3(0.0f, 0.0f, -1.0f));
    birdsEyeCam->SetMoveSpeed(100.0f);
    cameras[CameraType::BIRDS_EYE] = std::move(birdsEyeCam);

    // ORBIT Camera
    auto orbitCam = std::make_unique<Camera>();
    orbitCam->SetPosition(glm::vec3(150.0f, 0.0f, 0.0f)); // Further out
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
    // TODO: Clamp movement when near terrain or objects

    if (!activeCamera) return;

    // Compute exclusive (swapped) input mapping so one key set can't trigger both move and rotate.
    // Default (no CTRL): Group A (WASD) = movement, Group B (IJKL + Arrows) = rotation.
    // With CTRL held: groups swap roles (WASD = rotation, IJKL+Arrows = movement).
    const bool groupA_forward = keyW;
    const bool groupA_backward = keyS;
    const bool groupA_left = keyA;
    const bool groupA_right = keyD;
    const bool groupB_forward = keyI || keyUp;
    const bool groupB_backward = keyK || keyDown;
    const bool groupB_left = keyJ || keyLeft;
    const bool groupB_right = keyL || keyRight;

    // Effective movement flags
    const bool moveForward = keyCtrl ? groupB_forward : groupA_forward;
    const bool moveBackward = keyCtrl ? groupB_backward : groupA_backward;
    const bool moveLeft = keyCtrl ? groupB_left : groupA_left;
    const bool moveRight = keyCtrl ? groupB_right : groupA_right;

    // Vertical movement (Q/E) remain movement in both modes.
    const bool moveDown = keyQ;
    const bool moveUp = keyE;

    // Effective rotation flags (pitch = look up/down, yaw = look left/right)
    const bool rotatePitchUp = keyCtrl ? groupA_forward : groupB_forward;  // Look up
    const bool rotatePitchDown = keyCtrl ? groupA_backward : groupB_backward; // Look down
    const bool rotateYawLeft = keyCtrl ? groupA_left : groupB_left;     // Look left
    const bool rotateYawRight = keyCtrl ? groupA_right : groupB_right;    // Look right

    // Movement and rotation multipliers adjusted by SHIFT
    const float shiftMultiplier = keyShift ? 3.0f : 1.0f; // movement multiplier
    const float rotationMultiplier = keyShift ? 3.0f : 1.0f; // rotation multiplier

    const float moveDelta = deltaTime * shiftMultiplier;
    const float rotateDelta = deltaTime * rotationMultiplier;

    // Apply movement (exclusive)
    if (moveForward)  activeCamera->MoveForward(moveDelta);
    if (moveBackward) activeCamera->MoveBackward(moveDelta);
    if (moveLeft)     activeCamera->MoveLeft(moveDelta);
    if (moveRight)    activeCamera->MoveRight(moveDelta);
    if (moveDown)     activeCamera->MoveDown(moveDelta);
    if (moveUp)       activeCamera->MoveUp(moveDelta);

    // Apply rotation (exclusive) -- now affected by SHIFT multiplier
    if (rotatePitchUp)   activeCamera->RotatePitch(rotateDelta);
    if (rotatePitchDown) activeCamera->RotatePitch(-rotateDelta);
    if (rotateYawLeft)   activeCamera->RotateYaw(-rotateDelta);
    if (rotateYawRight)  activeCamera->RotateYaw(rotateDelta);
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

    // Modifiers
    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
        keyCtrl = pressed;
    }
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
        keyShift = pressed;
    }
}
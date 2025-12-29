#include "CameraController.h"
#include "Scene.h"
#include <GLFW/glfw3.h>
#include <algorithm> // For std::max
#include <iostream>

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

void CameraController::Update(float deltaTime, const Scene& scene) {
    if (activeCameraType == CameraType::FREE_ROAM) {
        UpdateFreeRoamCamera(deltaTime, scene);
    }
}

void CameraController::UpdateFreeRoamCamera(float deltaTime, const Scene& scene) {
    if (!activeCamera) return;

    // --- INPUT MAPPING ---
    const bool groupA_forward = keyW;
    const bool groupA_backward = keyS;
    const bool groupA_left = keyA;
    const bool groupA_right = keyD;
    const bool groupB_forward = keyI || keyUp;
    const bool groupB_backward = keyK || keyDown;
    const bool groupB_left = keyJ || keyLeft;
    const bool groupB_right = keyL || keyRight;

    // Effective movement flags (Swappable with CTRL)
    const bool moveForward = keyCtrl ? groupB_forward : groupA_forward;
    const bool moveBackward = keyCtrl ? groupB_backward : groupA_backward;
    const bool moveLeft = keyCtrl ? groupB_left : groupA_left;
    const bool moveRight = keyCtrl ? groupB_right : groupA_right;
    const bool moveDown = keyQ;
    const bool moveUp = keyE;

    // Effective rotation flags
    const bool rotatePitchUp = keyCtrl ? groupA_forward : groupB_forward;
    const bool rotatePitchDown = keyCtrl ? groupA_backward : groupB_backward;
    const bool rotateYawLeft = keyCtrl ? groupA_left : groupB_left;
    const bool rotateYawRight = keyCtrl ? groupA_right : groupB_right;

    const float shiftMultiplier = keyShift ? 3.0f : 1.0f;
    const float moveDelta = deltaTime * shiftMultiplier;
    const float rotateDelta = deltaTime * shiftMultiplier; // Applied rotation speed

    glm::vec3 oldPos = activeCamera->GetPosition();

    // --- 1. APPLY MOVEMENT ---
    if (moveForward)  activeCamera->MoveForward(moveDelta);
    if (moveBackward) activeCamera->MoveBackward(moveDelta);
    if (moveLeft)     activeCamera->MoveLeft(moveDelta);
    if (moveRight)    activeCamera->MoveRight(moveDelta);
    if (moveDown)     activeCamera->MoveDown(moveDelta);
    if (moveUp)       activeCamera->MoveUp(moveDelta);

    // --- 2. APPLY COLLISIONS ---
    // Retrieve the new candidate position, clamp it, and set it back.
    glm::vec3 currentPos = activeCamera->GetPosition();
    ClampCameraPosition(currentPos, scene, oldPos);
    activeCamera->SetPosition(currentPos);

    // --- 3. APPLY ROTATION ---
    if (rotatePitchUp)   activeCamera->RotatePitch(rotateDelta);
    if (rotatePitchDown) activeCamera->RotatePitch(-rotateDelta);
    if (rotateYawLeft)   activeCamera->RotateYaw(-rotateDelta);
    if (rotateYawRight)  activeCamera->RotateYaw(rotateDelta);
}

void CameraController::OnKeyPress(int key, bool pressed) {
    if (key == GLFW_KEY_W) keyW = pressed;
    if (key == GLFW_KEY_A) keyA = pressed;
    if (key == GLFW_KEY_S) keyS = pressed;
    if (key == GLFW_KEY_D) keyD = pressed;
    if (key == GLFW_KEY_Q) keyQ = pressed;
    if (key == GLFW_KEY_E) keyE = pressed;

    if (key == GLFW_KEY_I) keyI = pressed;
    if (key == GLFW_KEY_J) keyJ = pressed;
    if (key == GLFW_KEY_K) keyK = pressed;
    if (key == GLFW_KEY_L) keyL = pressed;

    if (key == GLFW_KEY_UP) keyUp = pressed;
    if (key == GLFW_KEY_LEFT) keyLeft = pressed;
    if (key == GLFW_KEY_DOWN) keyDown = pressed;
    if (key == GLFW_KEY_RIGHT) keyRight = pressed;

    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) keyCtrl = pressed;
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) keyShift = pressed;
}

void CameraController::ClampCameraPosition(glm::vec3& pos, const Scene& scene, const glm::vec3& prevPos) {
    const float COLLISION_BUFFER = 1.7f;

    // --- 1. DYNAMIC TERRAIN COLLISION ---
    const auto& terrain = scene.GetTerrainConfig();
    if (terrain.exists) {
        // 1. Calculate Local Coordinates
        float localX = pos.x - terrain.position.x;
        float localZ = pos.z - terrain.position.z;
        float distFromCenter = glm::length(glm::vec2(localX, localZ));

        // 2. Sample Height
        float rawNoiseHeight = GeometryGenerator::GetTerrainHeight(
            localX, localZ,
            terrain.radius,
            terrain.heightScale,
            terrain.noiseFreq
        );

        // 3. Convert to World Height
        float worldFloorY = rawNoiseHeight + terrain.position.y;
        float clampHeight = worldFloorY + COLLISION_BUFFER;

        if (distFromCenter < terrain.radius) {
            if (pos.y < clampHeight) {
                pos.y = clampHeight;
            }
        }
    }

    // --- 2. DYNAMIC OBJECT COLLISION ---
    for (const auto& obj : scene.GetObjects()) {
        if (!obj->hasCollision) continue;

        glm::vec3 objPos = glm::vec3(obj->transform[3]);
        float objTop = objPos.y + obj->collisionHeight;
        float bufferedTop = objTop + COLLISION_BUFFER;

        // Check horizontal distance first (Infinite cylinder check)
        float distXZ = glm::distance(glm::vec2(pos.x, pos.z), glm::vec2(objPos.x, objPos.z));
        float minSeparation = obj->collisionRadius + COLLISION_BUFFER;

        // Only process if we are horizontally intersecting
        if (distXZ < minSeparation) {

            // Check Vertical State
            bool isInsideVertical = (pos.y > objPos.y) && (pos.y < bufferedTop);
            bool wasAbove = (prevPos.y >= bufferedTop);

            if (isInsideVertical) {
                if (wasAbove) {
                    // CASE A: LANDING
                    pos.y = bufferedTop;
                }
                else {
                    // CASE B: WALL HIT
                    glm::vec2 dir = glm::vec2(pos.x, pos.z) - glm::vec2(objPos.x, objPos.z);
                    if (glm::length(dir) < 0.001f) dir = glm::vec2(1.0f, 0.0f);
                    else dir = glm::normalize(dir);

                    glm::vec2 corrected = glm::vec2(objPos.x, objPos.z) + dir * minSeparation;
                    pos.x = corrected.x;
                    pos.z = corrected.y;
                }
            }
        }
    }
}
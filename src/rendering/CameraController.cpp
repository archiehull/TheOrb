#include "CameraController.h"
#include "Scene.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <random>

CameraController::CameraController()
    : activeCamera(nullptr)
    , activeCameraType(CameraType::FREE_ROAM)
{
    SetupCameras();
    // Default pointer set to Birds Eye to match activeCameraType
    activeCamera = cameras[CameraType::FREE_ROAM].get();
}

void CameraController::SetupCameras() {
	// camera movement and rotation speed

    // 1. FREE ROAM Camera (F2) - Inside the Orb
    auto freeRoamCam = std::make_unique<Camera>();
    freeRoamCam->SetPosition(glm::vec3(0.0f, -75.0f, 0.0f));
    freeRoamCam->SetTarget(glm::vec3(0.0f, -75.0f, 10.0f)); // Look forward locally
    freeRoamCam->SetMoveSpeed(35.0f);
    freeRoamCam->SetRotateSpeed(45.0f);
    cameras[CameraType::OUTSIDE_STATIC] = std::move(freeRoamCam);

    // 2. STATIC Camera (F1) - Outer View
    auto staticCam = std::make_unique<Camera>();
    staticCam->SetPosition(glm::vec3(0.0f, 60.0f, 350.0f));
    staticCam->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    cameras[CameraType::FREE_ROAM] = std::move(staticCam);

    // 3. Orbit Camera (F3) - Cactus Focus
    auto OrbitCam = std::make_unique<Camera>();
    OrbitCam->SetPosition(glm::vec3(20.0f, 10.0f, 20.0f));
    OrbitCam->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    cameras[CameraType::CACTI] = std::move(OrbitCam);
}

void CameraController::SwitchCamera(CameraType type, const Scene& scene) {
    if (cameras.find(type) == cameras.end()) return;

    // Logic for initializing specific modes
    if (type == CameraType::CACTI) {
        // Find a random cactus
        std::vector<SceneObject*> cacti;
        for (const auto& obj : scene.GetObjects()) {
            if (obj->texturePath.find("cactus") != std::string::npos) {
                cacti.push_back(obj.get());
            }
        }

        if (!cacti.empty()) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, static_cast<int>(cacti.size()) - 1);

            OrbitTargetObject = cacti[dis(gen)];
            OrbitRadius = 15.0f;
            OrbitYaw = 0.0f;
            OrbitPitch = 20.0f;
            //std::cout << "Orbiting Cactus: " << OrbitTargetObject->name << std::endl;
        }
        else {
            std::cout << "No cactus found to Orbit!" << std::endl;
            type = CameraType::OUTSIDE_STATIC;
        }
    }
    else if (type == CameraType::OUTSIDE_STATIC) {
        // Reset to new default position
        auto* cam = cameras[CameraType::OUTSIDE_STATIC].get();
        cam->SetPosition(glm::vec3(85.0f, -65.0f, 35.0f));
        cam->SetTarget(glm::vec3(0.0f, -75.0f, 10.0f));
    }
    else if (type == CameraType::FREE_ROAM) {
        auto* cam = cameras[CameraType::FREE_ROAM].get();
        cam->SetPosition(glm::vec3(0.0f, 60.0f, 350.0f));
        cam->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    }

    activeCameraType = type;
    activeCamera = cameras[type].get();
}

void CameraController::Update(float deltaTime, const Scene& scene) {
    if (!activeCamera) return;

    switch (activeCameraType) {
    case CameraType::OUTSIDE_STATIC:
        UpdateFreeRoamCamera(deltaTime, scene);
        break;
    case CameraType::CACTI:
        UpdateOrbitCamera(deltaTime, scene);
        break;
    case CameraType::FREE_ROAM:
        break;
    }
}

void CameraController::UpdateOrbitCamera(float deltaTime, const Scene& scene) {
    if (!OrbitTargetObject) return;

    const float rotateSpeed = 50.0f;
    const float zoomSpeed = 20.0f;

    if (keyA || keyLeft)  OrbitYaw -= rotateSpeed * deltaTime;
    if (keyD || keyRight) OrbitYaw += rotateSpeed * deltaTime;
    if (keyW || keyUp)    OrbitPitch += rotateSpeed * deltaTime;
    if (keyS || keyDown)  OrbitPitch -= rotateSpeed * deltaTime;

    if (keyQ) OrbitRadius += zoomSpeed * deltaTime;
    if (keyE) OrbitRadius -= zoomSpeed * deltaTime;

    OrbitPitch = std::clamp(OrbitPitch, -10.0f, 89.0f);
    OrbitRadius = std::clamp(OrbitRadius, 5.0f, 50.0f);

    glm::vec3 targetPos = glm::vec3(OrbitTargetObject->transform[3]);
    targetPos.y += 3.0f;

    float radYaw = glm::radians(OrbitYaw);
    float radPitch = glm::radians(OrbitPitch);

    glm::vec3 offset;
    offset.x = OrbitRadius * cos(radPitch) * sin(radYaw);
    offset.y = OrbitRadius * sin(radPitch);
    offset.z = OrbitRadius * cos(radPitch) * cos(radYaw);

    glm::vec3 newPos = targetPos + offset;
    glm::vec3 oldPos = activeCamera->GetPosition();

    ClampCameraPosition(newPos, scene, oldPos);

    activeCamera->SetPosition(newPos);
    activeCamera->SetTarget(targetPos);
}

void CameraController::UpdateFreeRoamCamera(float deltaTime, const Scene& scene) {
    const bool groupA_forward = keyW;
    const bool groupA_backward = keyS;
    const bool groupA_left = keyA;
    const bool groupA_right = keyD;
    const bool groupB_forward = keyUp;
    const bool groupB_backward = keyDown;
    const bool groupB_left = keyLeft;
    const bool groupB_right = keyRight;

    const bool moveForward = keyCtrl ? groupB_forward : groupA_forward;
    const bool moveBackward = keyCtrl ? groupB_backward : groupA_backward;
    const bool moveLeft = keyCtrl ? groupB_left : groupA_left;
    const bool moveRight = keyCtrl ? groupB_right : groupA_right;
    const bool moveUp = keyQ; // incl pagedwn
    const bool moveDown = keyE; // incl pageup

    const bool rotatePitchUp = keyCtrl ? groupA_forward : groupB_forward;
    const bool rotatePitchDown = keyCtrl ? groupA_backward : groupB_backward;
    const bool rotateYawLeft = keyCtrl ? groupA_left : groupB_left;
    const bool rotateYawRight = keyCtrl ? groupA_right : groupB_right;

    const float shiftMultiplier = keyShift ? 3.0f : 1.0f;
    const float moveDelta = deltaTime * shiftMultiplier;
    const float rotateDelta = deltaTime * shiftMultiplier;

    glm::vec3 oldPos = activeCamera->GetPosition();

    if (moveForward)  activeCamera->MoveForward(moveDelta);
    if (moveBackward) activeCamera->MoveBackward(moveDelta);
    if (moveLeft)     activeCamera->MoveLeft(moveDelta);
    if (moveRight)    activeCamera->MoveRight(moveDelta);
    if (moveDown)     activeCamera->MoveDown(moveDelta);
    if (moveUp)       activeCamera->MoveUp(moveDelta);

    glm::vec3 currentPos = activeCamera->GetPosition();
    ClampCameraPosition(currentPos, scene, oldPos);
    activeCamera->SetPosition(currentPos);

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

    // Map PageDown to Q (Down) and PageUp to E (Up)
    if (key == GLFW_KEY_Q || key == GLFW_KEY_PAGE_UP) keyQ = pressed;
    if (key == GLFW_KEY_E || key == GLFW_KEY_PAGE_DOWN) keyE = pressed;

    // IJKL bindings removed for movement
    // if (key == GLFW_KEY_I) keyI = pressed;
    // if (key == GLFW_KEY_J) keyJ = pressed;
    // if (key == GLFW_KEY_K) keyK = pressed;
    // if (key == GLFW_KEY_L) keyL = pressed;

    if (key == GLFW_KEY_UP) keyUp = pressed;
    if (key == GLFW_KEY_LEFT) keyLeft = pressed;
    if (key == GLFW_KEY_DOWN) keyDown = pressed;
    if (key == GLFW_KEY_RIGHT) keyRight = pressed;

    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) keyCtrl = pressed;
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) keyShift = pressed;
}

void CameraController::ClampCameraPosition(glm::vec3& pos, const Scene& scene, const glm::vec3& prevPos) {
    const float COLLISION_BUFFER = 1.7f;

    const auto& terrain = scene.GetTerrainConfig();
    if (terrain.exists) {
        float localX = pos.x - terrain.position.x;
        float localZ = pos.z - terrain.position.z;
        float distFromCenter = glm::length(glm::vec2(localX, localZ));

        float rawNoiseHeight = GeometryGenerator::GetTerrainHeight(
            localX, localZ,
            terrain.radius,
            terrain.heightScale,
            terrain.noiseFreq
        );

        float worldFloorY = rawNoiseHeight + terrain.position.y;
        float clampHeight = worldFloorY + COLLISION_BUFFER;

        if (distFromCenter < terrain.radius) {
            if (pos.y < clampHeight) {
                pos.y = clampHeight;
            }
        }
    }

    for (const auto& obj : scene.GetObjects()) {
        if (!obj->hasCollision) continue;

        glm::vec3 objPos = glm::vec3(obj->transform[3]);
        float objTop = objPos.y + obj->collisionHeight;
        float bufferedTop = objTop + COLLISION_BUFFER;

        float distXZ = glm::distance(glm::vec2(pos.x, pos.z), glm::vec2(objPos.x, objPos.z));
        float minSeparation = obj->collisionRadius + COLLISION_BUFFER;

        if (distXZ < minSeparation) {
            bool isInsideVertical = (pos.y > objPos.y) && (pos.y < bufferedTop);
            bool wasAbove = (prevPos.y >= bufferedTop);

            if (isInsideVertical) {
                if (wasAbove) {
                    pos.y = bufferedTop;
                }
                else {
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
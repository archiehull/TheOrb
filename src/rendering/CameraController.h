#pragma once

#include "Camera.h"
#include <memory>
#include <map>
#include <vector>

class Scene;
struct SceneObject;

class CameraController final {
public:
    CameraController();
    ~CameraController() = default;

    // Non-copyable
    CameraController(const CameraController&) = delete;
    CameraController& operator=(const CameraController&) = delete;

    void Update(float deltaTime, const Scene& scene);

    inline Camera* GetActiveCamera() const { return activeCamera; }
    CameraType GetActiveCameraType() const { return activeCameraType; }

    // Updated to take Scene reference for finding objects (e.g. Cacti)
    void SwitchCamera(CameraType type, const Scene& scene);

    // NEW: Getter for the current CACTI target (for F4 Ignition)
    SceneObject* GetCACTITarget() const { return CACTITargetObject; }

    // Input handling
    void OnKeyPress(int key, bool pressed);
    inline void OnKeyRelease(int key) { OnKeyPress(key, false); }

private:
    std::map<CameraType, std::unique_ptr<Camera>> cameras;
    Camera* activeCamera = nullptr;
    CameraType activeCameraType = CameraType::OUTSIDE_STATIC;

    // Key states
    bool keyW = false, keyA = false, keyS = false, keyD = false;
    bool keyI = false, keyJ = false, keyK = false, keyL = false;
    bool keyQ = false, keyE = false;
    bool keyUp = false, keyDown = false, keyLeft = false, keyRight = false;
    bool keyCtrl = false;
    bool keyShift = false;

    // CACTI Camera State
    SceneObject* CACTITargetObject = nullptr; // Changed from const to mutable
    float CACTIRadius = 15.0f;
    float CACTIYaw = 0.0f;
    float CACTIPitch = 20.0f;

    void SetupCameras();
    void UpdateFreeRoamCamera(float deltaTime, const Scene& scene);
    void UpdateCACTICamera(float deltaTime, const Scene& scene);
    void ClampCameraPosition(glm::vec3& position, const Scene& scene, const glm::vec3& previousPosition);
};
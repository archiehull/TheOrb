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

    // NEW: Getter for the current Orbit target (for F4 Ignition)
    SceneObject* GetOrbitTarget() const { return OrbitTargetObject; }

    // Input handling
    void OnKeyPress(int key, bool pressed);
    inline void OnKeyRelease(int key) { OnKeyPress(key, false); }

private:
    std::map<CameraType, std::unique_ptr<Camera>> cameras;
    Camera* activeCamera = nullptr;
    CameraType activeCameraType = CameraType::FREE_ROAM;

    // Key states
    bool keyW = false, keyA = false, keyS = false, keyD = false;
    bool keyI = false, keyJ = false, keyK = false, keyL = false;
    bool keyQ = false, keyE = false;
    bool keyUp = false, keyDown = false, keyLeft = false, keyRight = false;
    bool keyCtrl = false;
    bool keyShift = false;

    // Orbit Camera State
    SceneObject* OrbitTargetObject = nullptr; // Changed from const to mutable
    float OrbitRadius = 15.0f;
    float OrbitYaw = 0.0f;
    float OrbitPitch = 20.0f;

    void SetupCameras();
    void UpdateFreeRoamCamera(float deltaTime, const Scene& scene);
    void UpdateOrbitCamera(float deltaTime, const Scene& scene);
    void ClampCameraPosition(glm::vec3& position, const Scene& scene, const glm::vec3& previousPosition) const;
};
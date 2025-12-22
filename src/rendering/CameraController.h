#pragma once

#include "Camera.h"
#include <memory>
#include <map>

class CameraController final {
public:
    CameraController();
    ~CameraController() = default;

    // Non-copyable
    CameraController(const CameraController&) = delete;
    CameraController& operator=(const CameraController&) = delete;

    // Movable
    //CameraController(CameraController&&) noexcept;
    //CameraController& operator=(CameraController&&) noexcept;

    void Update(float deltaTime);

    // Marked const to avoid exposing non-const accessors (resolves CODSTA-CPP.54)
    inline Camera* GetActiveCamera() const { return activeCamera; }
    CameraType GetActiveCameraType() const { return activeCameraType; }

    void SwitchCamera(CameraType type);

    // Input handling
    void OnKeyPress(int key, bool pressed);
    inline void OnKeyRelease(int key) { OnKeyPress(key, false); }

private:
    std::map<CameraType, std::unique_ptr<Camera>> cameras;
    Camera* activeCamera = nullptr;
    CameraType activeCameraType = CameraType::FREE_ROAM;

    // Key states for free roam camera
    bool keyW = false, keyA = false, keyS = false, keyD = false;
    bool keyI = false, keyJ = false, keyK = false, keyL = false;
    bool keyQ = false, keyE = false;
    bool keyUp = false, keyDown = false, keyLeft = false, keyRight = false;
    bool keyCtrl = false;
    bool keyShift = false;

    void SetupCameras();
    void UpdateFreeRoamCamera(float deltaTime);
};
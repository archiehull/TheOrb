#pragma once

#include "Camera.h"
#include <memory>
#include <map>

class CameraController {
public:
    CameraController();
    ~CameraController() = default;

    void Update(float deltaTime);

    Camera* GetActiveCamera() { return activeCamera; }
    CameraType GetActiveCameraType() const { return activeCameraType; }

    void SwitchCamera(CameraType type);

    // Input handling
    void OnKeyPress(int key, bool pressed);
    inline void OnKeyRelease(int key) { OnKeyPress(key, false); }

private:
    std::map<CameraType, std::unique_ptr<Camera>> cameras;
    Camera* activeCamera;
    CameraType activeCameraType;

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
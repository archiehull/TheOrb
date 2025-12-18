#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraType {
    FREE_ROAM,
    BIRDS_EYE,
    ORBIT
};

class Camera final {
public:
    Camera();
    ~Camera() = default;

    // Get view and projection matrices
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    // Camera controls
    inline void SetPosition(const glm::vec3& pos) { position = pos; }
    void SetTarget(const glm::vec3& target);
    void SetUp(const glm::vec3& up);

    // Movement (for free roam)
    void MoveForward(float delta);
    void MoveBackward(float delta);
    void MoveLeft(float delta);
    void MoveRight(float delta);
    void MoveUp(float delta);
    void MoveDown(float delta);

    // Rotation (for free roam)
    void RotateYaw(float delta);
    void RotatePitch(float delta);

    // Getters
    glm::vec3 GetPosition() const { return position; }
    glm::vec3 GetFront() const { return front; }
    glm::vec3 GetUp() const { return up; }

    // Camera settings
    void SetFOV(float fov) { this->fov = fov; }
    void SetNearFar(float nearPlane, float farPlane) {
        this->nearPlane = nearPlane;
        this->farPlane = farPlane;
    }
    void SetMoveSpeed(float speed) { moveSpeed = speed; }
    void SetRotateSpeed(float speed) { rotateSpeed = speed; }

private:
    // Camera vectors
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Euler angles
    float yaw;
    float pitch;

    // Camera settings
    float fov;
    float nearPlane;
    float farPlane;
    float moveSpeed;
    float rotateSpeed;

    // Update camera vectors based on euler angles
    void UpdateCameraVectors();
};
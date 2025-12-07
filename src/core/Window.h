#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

class Window {
public:
    Window(uint32_t width, uint32_t height, const std::string& title);
    ~Window();

    bool ShouldClose() const;
    void PollEvents() const;
    GLFWwindow* GetGLFWWindow() const { return window; }

    // Add callback setter
    void SetFramebufferResizeCallback(GLFWframebuffersizefun callback);

private:
    void initWindow();
    void cleanup();

    uint32_t width;
    uint32_t height;
    std::string title;
    GLFWwindow* window;
};
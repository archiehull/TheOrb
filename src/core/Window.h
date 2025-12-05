#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

class Window {
public:
    Window(uint32_t width, uint32_t height, const std::string& title);
    ~Window();

    // Delete copy constructor and assignment operator
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() const;
    void PollEvents() const;

    GLFWwindow* GetGLFWWindow() const { return window; }
    uint32_t GetWidth() const { return width; }
    uint32_t GetHeight() const { return height; }

private:
    void initWindow();
    void cleanup();

    GLFWwindow* window;
    uint32_t width;
    uint32_t height;
    std::string title;
};
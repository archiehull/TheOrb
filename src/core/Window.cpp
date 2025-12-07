#include "Window.h"
#include <stdexcept>

Window::Window(uint32_t width, uint32_t height, const std::string& title)
    : width(width), height(height), title(title), window(nullptr) {
    initWindow();
}

Window::~Window() {
    Cleanup();
}

void Window::initWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("failed to initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("failed to create GLFW window!");
    }
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(window);
}

void Window::PollEvents() const {
    glfwPollEvents();
}

void Window::SetFramebufferResizeCallback(GLFWframebuffersizefun callback) {
    glfwSetFramebufferSizeCallback(window, callback);
}

void Window::Cleanup() {
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}
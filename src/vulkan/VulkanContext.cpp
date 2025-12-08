#include "VulkanContext.h"
#include "VulkanUtils.h"
#include <stdexcept>

VulkanContext::VulkanContext() {
}

VulkanContext::~VulkanContext() {
}

void VulkanContext::CreateInstance() {
    if (VulkanUtils::enableValidationLayers && !VulkanUtils::CheckValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "The Orb";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = VulkanUtils::GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (VulkanUtils::enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanUtils::validationLayers.size());
        createInfo.ppEnabledLayerNames = VulkanUtils::validationLayers.data();

        VulkanUtils::PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void VulkanContext::SetupDebugMessenger() {
    if (!VulkanUtils::enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    VulkanUtils::PopulateDebugMessengerCreateInfo(createInfo);

    if (VulkanUtils::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VulkanContext::CreateSurface(GLFWwindow* window) {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void VulkanContext::Cleanup() {
    if (VulkanUtils::enableValidationLayers && debugMessenger != VK_NULL_HANDLE) {
        VulkanUtils::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }

    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }
}
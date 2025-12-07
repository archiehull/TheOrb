#include "VulkanShader.h"
#include <fstream>
#include <stdexcept>

VulkanShader::VulkanShader(VkDevice device) : device(device) {
}

VulkanShader::~VulkanShader() {
}

void VulkanShader::LoadShader(const std::string& filename, VkShaderStageFlagBits stage) {
    auto code = readFile(filename);
    VkShaderModule shaderModule = createShaderModule(code);

    if (stage == VK_SHADER_STAGE_VERTEX_BIT) {
        vertexShaderModule = shaderModule;
    }
    else if (stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
        fragmentShaderModule = shaderModule;
    }
}

VkShaderModule VulkanShader::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

std::vector<char> VulkanShader::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

void VulkanShader::Cleanup() {
    if (fragmentShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    }
    if (vertexShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    }
}
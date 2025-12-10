#pragma once

#include "Geometry.h"
#include <string>
#include <memory>
#include <vulkan/vulkan.h>

class OBJLoader {
public:
    static std::unique_ptr<Geometry> Load(VkDevice device, VkPhysicalDevice physicalDevice, const std::string& filepath);
};
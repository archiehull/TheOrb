#pragma once

#include <vulkan/vulkan.h>
#include <string>

class Pipeline {
public:
    Pipeline(VkDevice device);
    virtual ~Pipeline();

    virtual void Create() = 0;  // Pure virtual
    virtual void Cleanup();

    VkPipeline GetPipeline() const { return pipeline; }
    VkPipelineLayout GetLayout() const { return pipelineLayout; }

protected:
    VkDevice device;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};
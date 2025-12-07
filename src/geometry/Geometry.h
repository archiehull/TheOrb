#pragma once

#include "../vulkan/Vertex.h"
#include "../vulkan/VulkanBuffer.h"
#include <vector>
#include <memory>

class Geometry {
public:
    Geometry(VkDevice device, VkPhysicalDevice physicalDevice);
    ~Geometry();

    void CreateBuffers();
    void Bind(VkCommandBuffer commandBuffer);
    void Draw(VkCommandBuffer commandBuffer);
    void Cleanup();

    // Allow GeometryGenerator to populate data
    std::vector<Vertex>& GetVertices() { return vertices; }
    std::vector<uint32_t>& GetIndices() { return indices; }

    const std::vector<Vertex>& GetVertices() const { return vertices; }
    const std::vector<uint32_t>& GetIndices() const { return indices; }

    bool HasIndices() const { return !indices.empty(); }

protected:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkDevice device;
    VkPhysicalDevice physicalDevice;

    std::unique_ptr<VulkanBuffer> vertexBuffer;
    std::unique_ptr<VulkanBuffer> indexBuffer;
};
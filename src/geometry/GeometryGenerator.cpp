#include "GeometryGenerator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper to set normal
void SetNormal(Vertex& v, glm::vec3 n) { v.normal = n; }

std::unique_ptr<Geometry> GeometryGenerator::CreateTriangle(VkDevice device, VkPhysicalDevice physicalDevice) {
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);
    geometry->GetVertices().resize(3);
    // Flat triangle in XY plane, normal is +Z
    geometry->GetVertices()[0] = { glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
    geometry->GetVertices()[1] = { glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
    geometry->GetVertices()[2] = { glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
    geometry->CreateBuffers();
    return geometry;
}

std::unique_ptr<Geometry> GeometryGenerator::CreateQuad(VkDevice device, VkPhysicalDevice physicalDevice) {
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);
    geometry->GetVertices().resize(4);
    // Flat quad in XY plane, normal is +Z
    geometry->GetVertices()[0] = { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
    geometry->GetVertices()[1] = { glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
    geometry->GetVertices()[2] = { glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
    geometry->GetVertices()[3] = { glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) };

    geometry->GetIndices() = { 0, 1, 2, 2, 3, 0 };
    geometry->CreateBuffers();
    return geometry;
}

std::unique_ptr<Geometry> GeometryGenerator::CreateCircle(VkDevice device, VkPhysicalDevice physicalDevice, int segments, float radius) {
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);
    // Center
    geometry->GetVertices().push_back({ glm::vec3(0.0f), glm::vec3(1.0f), glm::vec2(0.5f), glm::vec3(0.0f, 0.0f, 1.0f) });

    for (int i = 0; i <= segments; ++i) {
        float angle = (2.0f * M_PI * i) / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        glm::vec3 color = GenerateColor(i, segments);
        glm::vec2 uv = glm::vec2((x / radius + 1.0f) * 0.5f, (y / radius + 1.0f) * 0.5f);
        geometry->GetVertices().push_back({ glm::vec3(x, y, 0.0f), color, uv, glm::vec3(0.0f, 0.0f, 1.0f) });
    }

    for (int i = 1; i <= segments; ++i) {
        geometry->GetIndices().push_back(0);
        geometry->GetIndices().push_back(i);
        geometry->GetIndices().push_back(i + 1);
    }
    geometry->CreateBuffers();
    return geometry;
}

std::unique_ptr<Geometry> GeometryGenerator::CreateCube(VkDevice device, VkPhysicalDevice physicalDevice) {
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);
    std::vector<Vertex>& verts = geometry->GetVertices();

    // Helper to add face with specific normal
    auto AddFace = [&](glm::vec3 n, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3, glm::vec2 uv4) {
        verts.push_back({ p1, glm::vec3(1.0f), uv1, n });
        verts.push_back({ p2, glm::vec3(1.0f), uv2, n });
        verts.push_back({ p3, glm::vec3(1.0f), uv3, n });
        verts.push_back({ p4, glm::vec3(1.0f), uv4, n });
        };

    // Front Face (+Z)
    AddFace(glm::vec3(0, 0, 1),
        glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, 0.5f),
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 });

    // Back Face (-Z)
    AddFace(glm::vec3(0, 0, -1),
        glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0.5f, 0.5f, -0.5f),
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 });

    // Top Face (+Y)
    AddFace(glm::vec3(0, 1, 0),
        glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(-0.5f, 0.5f, -0.5f),
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 });

    // Bottom Face (-Y)
    AddFace(glm::vec3(0, -1, 0),
        glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(-0.5f, -0.5f, 0.5f),
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 });

    // Right Face (+X)
    AddFace(glm::vec3(1, 0, 0),
        glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.5f, 0.5f, 0.5f),
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 });

    // Left Face (-X)
    AddFace(glm::vec3(-1, 0, 0),
        glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, -0.5f),
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 });

    geometry->GetIndices() = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };

    geometry->CreateBuffers();
    return geometry;
}

std::unique_ptr<Geometry> GeometryGenerator::CreateGrid(VkDevice device, VkPhysicalDevice physicalDevice, int rows, int cols, float cellSize) {
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);
    float width = cols * cellSize;
    float height = rows * cellSize;
    float startX = -width / 2.0f;
    float startY = -height / 2.0f;

    for (int row = 0; row <= rows; ++row) {
        for (int col = 0; col <= cols; ++col) {
            float x = startX + col * cellSize;
            float y = startY + row * cellSize;
            glm::vec3 color = GenerateColor(row * (cols + 1) + col, (rows + 1) * (cols + 1));
            glm::vec2 uv = glm::vec2((float)col / cols, (float)row / rows);
            // Grid in XZ plane, Normal is +Y
            geometry->GetVertices().push_back({ glm::vec3(x, 0.0f, y), color, uv, glm::vec3(0.0f, 1.0f, 0.0f) });
        }
    }
    // Indices generation remains the same as your original file...
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            uint32_t topLeft = row * (cols + 1) + col;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (row + 1) * (cols + 1) + col;
            uint32_t bottomRight = bottomLeft + 1;
            geometry->GetIndices().push_back(topLeft);
            geometry->GetIndices().push_back(bottomLeft);
            geometry->GetIndices().push_back(topRight);
            geometry->GetIndices().push_back(topRight);
            geometry->GetIndices().push_back(bottomLeft);
            geometry->GetIndices().push_back(bottomRight);
        }
    }
    geometry->CreateBuffers();
    return geometry;
}

std::unique_ptr<Geometry> GeometryGenerator::CreateSphere(VkDevice device, VkPhysicalDevice physicalDevice, int stacks, int slices, float radius) {
    if (stacks < 2) stacks = 2;
    if (slices < 3) slices = 3;
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);

    for (int i = 0; i <= stacks; ++i) {
        float phi = M_PI * i / stacks;
        float y = radius * cos(phi);
        float sinPhi = sin(phi);

        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * M_PI * j / slices;
            float x = radius * sinPhi * cos(theta);
            float z = radius * sinPhi * sin(theta);

            glm::vec3 pos = glm::vec3(x, y, z);
            glm::vec3 normal = glm::normalize(pos); // Sphere normal is just normalized position
            glm::vec3 color = GenerateColor(i * (slices + 1) + j, (stacks + 1) * (slices + 1));
            glm::vec2 uv = glm::vec2((float)j / slices, 1.0f - (float)i / stacks);

            geometry->GetVertices().push_back({ pos, color, uv, normal });
        }
    }
    // Indices generation remains same as your original file...
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t first = i * (slices + 1) + j;
            uint32_t second = first + (slices + 1);
            geometry->GetIndices().push_back(first);
            geometry->GetIndices().push_back(first + 1);
            geometry->GetIndices().push_back(second);
            geometry->GetIndices().push_back(first + 1);
            geometry->GetIndices().push_back(second + 1);
            geometry->GetIndices().push_back(second);
        }
    }
    geometry->CreateBuffers();
    return geometry;
}
// GenerateColor remains the same
glm::vec3 GeometryGenerator::GenerateColor(int index, int total) {
    // ... (keep existing implementation)
    float hue = (float)index / (float)total;
    float r, g, b;
    if (hue < 0.33f) { r = 1.0f - (hue / 0.33f); g = hue / 0.33f; b = 0.0f; }
    else if (hue < 0.66f) { r = 0.0f; g = 1.0f - ((hue - 0.33f) / 0.33f); b = (hue - 0.33f) / 0.33f; }
    else { r = (hue - 0.66f) / 0.34f; g = 0.0f; b = 1.0f - ((hue - 0.66f) / 0.34f); }
    return glm::vec3(r, g, b);
}
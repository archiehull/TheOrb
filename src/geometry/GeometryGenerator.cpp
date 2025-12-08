#include "GeometryGenerator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::unique_ptr<Geometry> GeometryGenerator::CreateTriangle(VkDevice device, VkPhysicalDevice physicalDevice) {
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);

    geometry->GetVertices().resize(3);
    geometry->GetVertices()[0] = { glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) };
    geometry->GetVertices()[1] = { glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
    geometry->GetVertices()[2] = { glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };

    geometry->CreateBuffers();
    return geometry;
}

std::unique_ptr<Geometry> GeometryGenerator::CreateQuad(VkDevice device, VkPhysicalDevice physicalDevice) {
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);

    // Define 4 vertices
    geometry->GetVertices().resize(4);
    geometry->GetVertices()[0] = { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) };
    geometry->GetVertices()[1] = { glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
    geometry->GetVertices()[2] = { glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
    geometry->GetVertices()[3] = { glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f) };

    // Define indices
    geometry->GetIndices() = {
        0, 1, 2,
        2, 3, 0
    };

    geometry->CreateBuffers();
    return geometry;
}

std::unique_ptr<Geometry> GeometryGenerator::CreateCircle(VkDevice device, VkPhysicalDevice physicalDevice,
    int segments, float radius) {
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);

    // Center vertex
    geometry->GetVertices().push_back({ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f) });

    // Create vertices around the circle
    for (int i = 0; i <= segments; ++i) {
        float angle = (2.0f * M_PI * i) / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);

        glm::vec3 color = GenerateColor(i, segments);
        geometry->GetVertices().push_back({ glm::vec3(x, y, 0.0f), color });
    }

    // Create indices for triangle fan
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

    // 24 vertices (4 per face) with consistent ordering: BL, BR, TR, TL for each face
    verts = {
        // Front face (+Z)
        { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) }, // 0 BL
        { glm::vec3(0.5f, -0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) }, // 1 BR
        { glm::vec3(0.5f,  0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) }, // 2 TR
        { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) }, // 3 TL

        // Back face (-Z)  (ordered so outside view sees CCW)
        { glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) }, // 4 BL (from back)
        { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) }, // 5 BR
        { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) }, // 6 TR
        { glm::vec3(0.5f,  0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) }, // 7 TL

        // Top face (+Y)
        { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) }, // 8 BL
        { glm::vec3(0.5f,  0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) }, // 9 BR
        { glm::vec3(0.5f,  0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) }, //10 TR
        { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) }, //11 TL

        // Bottom face (-Y)
        { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) }, //12 BL
        { glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) }, //13 BR
        { glm::vec3(0.5f, -0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) }, //14 TR
        { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) }, //15 TL

        // Right face (+X)
        { glm::vec3(0.5f, -0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) }, //16 BL
        { glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) }, //17 BR
        { glm::vec3(0.5f,  0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) }, //18 TR
        { glm::vec3(0.5f,  0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) }, //19 TL

        // Left face (-X)
        { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) }, //20 BL
        { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) }, //21 BR
        { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) }, //22 TR
        { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) }  //23 TL
    };

    // Each face: two triangles (BL, BR, TR) and (TR, TL, BL) => consistent CCW from outside
    geometry->GetIndices() = {
        0, 1, 2,   2, 3, 0,    // Front
        4, 5, 6,   6, 7, 4,    // Back
        8, 9,10,  10,11, 8,    // Top
       12,13,14,  14,15,12,    // Bottom
       16,17,18,  18,19,16,    // Right
       20,21,22,  22,23,20     // Left
    };

    geometry->CreateBuffers();
    return geometry;
}

std::unique_ptr<Geometry> GeometryGenerator::CreateGrid(VkDevice device, VkPhysicalDevice physicalDevice,
    int rows, int cols, float cellSize) {
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);

    float width = cols * cellSize;
    float height = rows * cellSize;
    float startX = -width / 2.0f;
    float startY = -height / 2.0f;

    // Generate vertices (including UVs)
    for (int row = 0; row <= rows; ++row) {
        for (int col = 0; col <= cols; ++col) {
            float x = startX + col * cellSize;
            float y = startY + row * cellSize;

            glm::vec3 color = GenerateColor(row * (cols + 1) + col, (rows + 1) * (cols + 1));
            glm::vec2 uv = glm::vec2(static_cast<float>(col) / static_cast<float>(cols),
                static_cast<float>(row) / static_cast<float>(rows));
            // Place grid in the XZ plane (horizontal) at y = 0
            geometry->GetVertices().push_back({ glm::vec3(x, 0.0f, y), color, uv });
        }
    }

    // Generate indices
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

glm::vec3 GeometryGenerator::GenerateColor(int index, int total) {
    float hue = (float)index / (float)total;
    float r, g, b;

    if (hue < 0.33f) {
        r = 1.0f - (hue / 0.33f);
        g = hue / 0.33f;
        b = 0.0f;
    }
    else if (hue < 0.66f) {
        r = 0.0f;
        g = 1.0f - ((hue - 0.33f) / 0.33f);
        b = (hue - 0.33f) / 0.33f;
    }
    else {
        r = (hue - 0.66f) / 0.34f;
        g = 0.0f;
        b = 1.0f - ((hue - 0.66f) / 0.34f);
    }

    return glm::vec3(r, g, b);
}
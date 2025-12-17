#include "GeometryGenerator.h"
#include <cmath>
#include <glm/gtc/noise.hpp>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper to set normal
void SetNormal(Vertex& v, glm::vec3 n) { v.normal = n; }

float SmoothStep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float GeometryGenerator::GetTerrainHeight(float x, float z, float radius, float heightScale, float noiseFreq) {
    float dist = glm::length(glm::vec2(x, z));

    // 1. Noise Generation
    float y = 0.0f;
    // Base layer
    y += glm::perlin(glm::vec2(x, z) * noiseFreq);
    // Detail layer
    y += glm::perlin(glm::vec2(x, z) * noiseFreq * 2.0f) * 0.25f;

    y *= heightScale;

    // 2. Circular Clamping (Masking)
    // Matches the visual mesh generation
    float edgeFactor = dist / radius;
    // We apply the mask slightly aggressively to ensure objects don't spawn floating off the very edge
    if (edgeFactor > 0.95f) {
        y *= 0.0f; // Force flat/down at edges
    }
    else {
        // Same fade as CreateTerrain
        // Note: CreateTerrain used the loop index for fading, here we use distance
        // approximating the visual fade effect:
        if (edgeFactor > 0.9f) {
            y *= 1.0f - ((edgeFactor - 0.9f) * 10.0f);
        }
    }

    return y;
}

std::unique_ptr<Geometry> GeometryGenerator::CreateBowl(VkDevice device, VkPhysicalDevice physicalDevice, float radius, int slices, int stacks) {
    auto geometry = std::make_unique<Geometry>(device, physicalDevice);

    // Generate Bottom Hemisphere (Phi: PI/2 -> PI)
    for (int i = 0; i <= stacks; ++i) {
        // Interpolate v from 0 to 1
        float v = (float)i / stacks;

        // phi goes from PI/2 (Equator) to PI (South Pole)
        float phi = M_PI * 0.5f + (M_PI * 0.5f * v);

        float y = radius * cos(phi);
        float r_at_y = radius * sin(phi);

        for (int j = 0; j <= slices; ++j) {
            float u = (float)j / slices;
            float theta = 2.0f * M_PI * u;

            float x = r_at_y * cos(theta);
            float z = r_at_y * sin(theta);

            glm::vec3 pos(x, y, z);
            glm::vec3 normal = glm::normalize(pos);

            // Texture coordinates: u wraps around, v goes top-to-bottom of the bowl
            glm::vec2 uv(u, v);

            // Simple white/grey color base
            glm::vec3 color(0.8f, 0.8f, 0.8f);

            geometry->GetVertices().push_back({ pos, color, uv, normal });
        }
    }

    // Indices (Standard Grid Logic)
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

std::unique_ptr<Geometry> GeometryGenerator::CreatePedestal(VkDevice device, VkPhysicalDevice physicalDevice,
    float topRadius, float baseWidth, float height, int slices, int stacks) {

    auto geometry = std::make_unique<Geometry>(device, physicalDevice);
    std::vector<Vertex>& vertices = geometry->GetVertices();
    std::vector<uint32_t>& indices = geometry->GetIndices();

    for (int i = 0; i <= stacks; ++i) {
        float v = (float)i / stacks;
        float y = -v * height; // Goes down from 0 to -height

        for (int j = 0; j <= slices; ++j) {
            float u = (float)j / slices;
            float theta = 2.0f * M_PI * u;

            // 1. Calculate Circular Top Position
            float x_circle = topRadius * cos(theta);
            float z_circle = topRadius * sin(theta);

            // 2. Calculate Circular Bottom Position (CHANGED)
            // Previously this calculated a square profile. 
            // Now we treat baseWidth as the diameter of a circle.
            float baseRadius = baseWidth * 0.5f;
            float x_base = baseRadius * cos(theta);
            float z_base = baseRadius * sin(theta);

            // 3. Interpolate (Linear Loft)
            glm::vec3 pos;
            // Linearly interpolate between the top circle and bottom circle
            pos.x = glm::mix(x_circle, x_base, v);
            pos.y = y;
            pos.z = glm::mix(z_circle, z_base, v);

            glm::vec2 uv(u, v);
            glm::vec3 color(0.8f, 0.8f, 0.8f);
            glm::vec3 normal(0.0f, 1.0f, 0.0f); // Placeholder, computed below

            vertices.push_back({ pos, color, uv, normal });
        }
    }

    // Indices (Standard Grid Logic)
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t current = i * (slices + 1) + j;
            uint32_t next = current + (slices + 1);

            indices.push_back(current);
            indices.push_back(current + 1);
            indices.push_back(next);

            indices.push_back(current + 1);
            indices.push_back(next + 1);
            indices.push_back(next);
        }
    }

    // Compute Normals (Smooth)
    for (auto& v : vertices) v.normal = glm::vec3(0.0f);
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        glm::vec3 v0 = vertices[i0].pos;
        glm::vec3 v1 = vertices[i1].pos;
        glm::vec3 v2 = vertices[i2].pos;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::cross(edge1, edge2);

        vertices[i0].normal += normal;
        vertices[i1].normal += normal;
        vertices[i2].normal += normal;
    }
    for (auto& v : vertices) {
        if (glm::length(v.normal) > 0.00001f)
            v.normal = glm::normalize(v.normal);
        else
            v.normal = glm::vec3(0, 1, 0);
    }

    geometry->CreateBuffers();
    return geometry;
}

std::unique_ptr<Geometry> GeometryGenerator::CreateTerrain(VkDevice device, VkPhysicalDevice physicalDevice,
    float radius, int rings, int segments, float heightScale, float noiseFreq) {

    auto geometry = std::make_unique<Geometry>(device, physicalDevice);
    std::vector<Vertex>& vertices = geometry->GetVertices();
    std::vector<uint32_t>& indices = geometry->GetIndices();

    for (int i = 0; i <= rings; ++i) {
        float r = (float)i / rings * radius;

        for (int j = 0; j <= segments; ++j) {
            float theta = (float)j / segments * 2.0f * M_PI;

            float x = r * cos(theta);
            float z = r * sin(theta);

            // Re-use the centralized height logic if you implemented GetTerrainHeight, 
            // otherwise keep the noise logic here.
            // For this snippet, I'll assume the noise logic is inline or called via helper.
            float y = 0.0f;
            y += glm::perlin(glm::vec2(x, z) * noiseFreq);
            y += glm::perlin(glm::vec2(x, z) * noiseFreq * 2.0f) * 0.25f;
            y *= heightScale;

            // Apply edge mask (inline logic for consistency with previous file)
            float edgeFactor = (float)i / rings;
            if (edgeFactor > 0.9f) {
                float mask = 1.0f - ((edgeFactor - 0.9f) * 10.0f);
                y *= mask;
            }
            if (i == 0) y = 0.0f;

            glm::vec3 pos(x, y, z);

            // Coloring
            float hFactor = (y / heightScale) + 0.5f;
            glm::vec3 lowColor = glm::vec3(0.35f, 0.30f, 0.25f);
            glm::vec3 highColor = glm::vec3(0.45f, 0.40f, 0.30f);
            glm::vec3 color = glm::mix(lowColor, highColor, hFactor);
            if (edgeFactor > 0.9f) color *= (1.0f - ((edgeFactor - 0.9f) * 10.0f));

            // UV Mapping with Tiling
            glm::vec2 uv;
            // Original 0..1 range
            uv.x = (x / radius) * 0.5f + 0.5f;
            uv.y = (z / radius) * 0.5f + 0.5f;


            float textureTiling = 80.0f;
            // Apply tiling
            uv *= textureTiling;

            glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vertices.push_back({ pos, color, uv, normal });
        }
    }

    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            uint32_t current = i * (segments + 1) + j;
            uint32_t next = current + (segments + 1);

            indices.push_back(current);
            indices.push_back(current + 1);
            indices.push_back(next);

            indices.push_back(current + 1);
            indices.push_back(next + 1);
            indices.push_back(next);
        }
    }

    for (auto& v : vertices) v.normal = glm::vec3(0.0f);
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];
        glm::vec3 v0 = vertices[i0].pos;
        glm::vec3 v1 = vertices[i1].pos;
        glm::vec3 v2 = vertices[i2].pos;
        glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
        vertices[i0].normal += normal;
        vertices[i1].normal += normal;
        vertices[i2].normal += normal;
    }
    for (auto& v : vertices) {
        if (glm::length(v.normal) > 0.0001f) v.normal = glm::normalize(v.normal);
        else v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
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
    float hue = (float)index / (float)total;
    float r, g, b;
    if (hue < 0.33f) { r = 1.0f - (hue / 0.33f); g = hue / 0.33f; b = 0.0f; }
    else if (hue < 0.66f) { r = 0.0f; g = 1.0f - ((hue - 0.33f) / 0.33f); b = (hue - 0.33f) / 0.33f; }
    else { r = (hue - 0.66f) / 0.34f; g = 0.0f; b = 1.0f - ((hue - 0.66f) / 0.34f); }
    return glm::vec3(r, g, b);
}
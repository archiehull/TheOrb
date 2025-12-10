#include "OBJLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>

// Hashing helper for vertex data deduplication
struct VertexKey {
    int v_idx = -1;
    int vt_idx = -1;
    int vn_idx = -1; // Parsed but unused in your current Vertex struct

    bool operator==(const VertexKey& other) const {
        return v_idx == other.v_idx && vt_idx == other.vt_idx && vn_idx == other.vn_idx;
    }
};

struct VertexKeyHash {
    size_t operator()(const VertexKey& k) const {
        // Simple hash combination
        size_t h1 = std::hash<int>{}(k.v_idx);
        size_t h2 = std::hash<int>{}(k.vt_idx);
        size_t h3 = std::hash<int>{}(k.vn_idx);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

std::unique_ptr<Geometry> OBJLoader::Load(VkDevice device, VkPhysicalDevice physicalDevice, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open OBJ file: " + filepath);
    }

    // Temporary storage for raw file data
    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec2> temp_texCoords;
    // std::vector<glm::vec3> temp_normals; // Unused by Vertex struct currently

    // Deduplication map: Key -> Index in the final geometry vertex array
    std::unordered_map<VertexKey, uint32_t, VertexKeyHash> uniqueVertices;

    auto geometry = std::make_unique<Geometry>(device, physicalDevice);
    std::vector<Vertex>& outVertices = geometry->GetVertices();
    std::vector<uint32_t>& outIndices = geometry->GetIndices();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            glm::vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            temp_positions.push_back(pos);
        }
        else if (prefix == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            // Vulkan UV coordinate system has (0,0) at top-left.
            // OBJ (0,0) is bottom-left. Flip V.
            uv.y = 1.0f - uv.y;
            temp_texCoords.push_back(uv);
        }
        else if (prefix == "vn") {
            // Normals ignored for now as Vertex struct doesn't support them
            // glm::vec3 norm;
            // ss >> norm.x >> norm.y >> norm.z;
            // temp_normals.push_back(norm);
        }
        else if (prefix == "f") {
            std::vector<VertexKey> faceVertices;
            std::string segment;

            // Parse all vertices in the face (supports triangles and quads)
            while (ss >> segment) {
                VertexKey key;
                std::stringstream segmentSS(segment);
                std::string valStr;
                int val;

                // 1. Position Index
                if (std::getline(segmentSS, valStr, '/')) {
                    if (!valStr.empty()) key.v_idx = std::stoi(valStr) - 1; // OBJ is 1-based
                }

                // 2. Texture Index
                if (std::getline(segmentSS, valStr, '/')) {
                    if (!valStr.empty()) key.vt_idx = std::stoi(valStr) - 1;
                }

                // 3. Normal Index
                if (std::getline(segmentSS, valStr, '/')) {
                    if (!valStr.empty()) key.vn_idx = std::stoi(valStr) - 1;
                }

                faceVertices.push_back(key);
            }

            // Triangulate (Fan triangulation: 0-1-2, 0-2-3, etc.)
            for (size_t i = 1; i < faceVertices.size() - 1; ++i) {
                VertexKey keys[3] = { faceVertices[0], faceVertices[i], faceVertices[i + 1] };

                for (int k = 0; k < 3; ++k) {
                    if (uniqueVertices.count(keys[k]) == 0) {
                        uniqueVertices[keys[k]] = static_cast<uint32_t>(outVertices.size());

                        Vertex newVertex{};

                        // Set Position
                        if (keys[k].v_idx >= 0 && keys[k].v_idx < temp_positions.size()) {
                            newVertex.pos = temp_positions[keys[k].v_idx];
                        }

                        // Set TexCoord (default 0,0 if missing)
                        if (keys[k].vt_idx >= 0 && keys[k].vt_idx < temp_texCoords.size()) {
                            newVertex.texCoord = temp_texCoords[keys[k].vt_idx];
                        }

                        // Set Default Color (White) since OBJ usually doesn't provide it per vertex
                        newVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);

                        outVertices.push_back(newVertex);
                    }

                    outIndices.push_back(uniqueVertices[keys[k]]);
                }
            }
        }
    }

    if (outVertices.empty()) {
        throw std::runtime_error("OBJ file contained no vertices or failed to parse: " + filepath);
    }

    geometry->CreateBuffers();
    return geometry;
}
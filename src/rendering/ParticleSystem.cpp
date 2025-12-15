#include "ParticleSystem.h"
#include <random>
#include <algorithm> 
#include <iostream>

// Helper for random numbers
static float RandomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(mt);
}

ParticleSystem::ParticleSystem(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, uint32_t maxParticles, uint32_t framesInFlight)
    : device(device), physicalDevice(physicalDevice), maxParticles(maxParticles), framesInFlight(framesInFlight) {
    particles.resize(maxParticles);
    poolIndex = maxParticles - 1;
    texture = std::make_unique<Texture>(device, physicalDevice, commandPool, graphicsQueue);
}

ParticleSystem::~ParticleSystem() {
    // Only destroy the descriptor pool if it was created.
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }

    texture.reset();
    vertexBuffer.reset();
    // Clear vector
    instanceBuffers.clear();
}

void ParticleSystem::Initialize(VkDescriptorSetLayout textureLayout, GraphicsPipeline* pipeline, const std::string& texturePath) {
    this->texturePath = texturePath;
    this->textureLayout = textureLayout;
    this->pipeline = pipeline;

    texture->LoadFromFile(texturePath);
    SetupBuffers();

    // Pool & Set creation remains here because each system needs its own descriptor set (for its specific texture)
    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create particle descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        // Clean up pool if allocation failed to avoid leaving a dangling pool
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
        throw std::runtime_error("failed to allocate particle descriptor set!");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->GetImageView();
    imageInfo.sampler = texture->GetSampler();

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void ParticleSystem::SetSimulationBounds(const glm::vec3& center, float radius) {
    this->boundsCenter = center;
    this->boundsRadius = radius;
    this->useBounds = true;
}

void ParticleSystem::Emit(const ParticleProps& props) {
    Particle& p = particles[poolIndex];
    p.active = true;

    // NEW: Position Jitter (Box Emitter)
    p.position.x = props.position.x + props.positionVariation.x * RandomFloat(-1.0f, 1.0f);
    p.position.y = props.position.y + props.positionVariation.y * RandomFloat(-1.0f, 1.0f);
    p.position.z = props.position.z + props.positionVariation.z * RandomFloat(-1.0f, 1.0f);

    p.velocity = props.velocity;
    p.velocity.x += props.velocityVariation.x * RandomFloat(-1.0f, 1.0f);
    p.velocity.y += props.velocityVariation.y * RandomFloat(-1.0f, 1.0f);
    p.velocity.z += props.velocityVariation.z * RandomFloat(-1.0f, 1.0f);

    p.colorBegin = props.colorBegin;
    p.colorEnd = props.colorEnd;
    p.lifeTime = props.lifeTime;
    p.lifeRemaining = props.lifeTime;
    p.sizeBegin = props.sizeBegin + props.sizeVariation * RandomFloat(-1.0f, 1.0f);
    p.sizeEnd = props.sizeEnd;

    if (poolIndex == 0) {
        poolIndex = maxParticles - 1;
    }
    else {
        poolIndex--;
    }
}

void ParticleSystem::AddEmitter(const ParticleProps& props, float particlesPerSecond) {
    ParticleEmitter emitter;
    emitter.props = props;
    emitter.particlesPerSecond = particlesPerSecond;
    emitter.timeSinceLastEmit = 0.0f;
    emitters.push_back(emitter);
}

void ParticleSystem::Update(float dt) {
    for (auto& emitter : emitters) {
        emitter.timeSinceLastEmit += dt;
        float emitInterval = 1.0f / emitter.particlesPerSecond;
        float maxTime = 0.1f;
        if (emitter.timeSinceLastEmit > maxTime) emitter.timeSinceLastEmit = maxTime;
        while (emitter.timeSinceLastEmit >= emitInterval) {
            Emit(emitter.props);
            emitter.timeSinceLastEmit -= emitInterval;
        }
    }
    for (auto& p : particles) {
        if (!p.active) continue;
        if (p.lifeRemaining <= 0.0f) {
            p.active = false;
            continue;
        }
        p.lifeRemaining -= dt;
        p.position += p.velocity * dt;

        // --- Clamping Logic ---
        if (useBounds) {
            float dist = glm::distance(p.position, boundsCenter);
            // If outside the radius, pull back to surface
            if (dist > boundsRadius) {
                // Avoid division by zero
                if (dist > 0.0001f) {
                    glm::vec3 dir = (p.position - boundsCenter) / dist;
                    p.position = boundsCenter + dir * boundsRadius;
                }
            }
        }
    }
}

void ParticleSystem::UpdateInstanceBuffer(uint32_t currentFrame) {
    std::vector<InstanceData> instanceData;
    instanceData.reserve(maxParticles);

    for (const auto& p : particles) {
        if (!p.active) continue;

        float lifeT = 1.0f - (p.lifeRemaining / p.lifeTime);

        InstanceData data{}; // Zero initialize

        // Explicitly convert vec3 -> vec4 (w=1.0)
        data.position = glm::vec4(p.position, 1.0f);

        // Color is already vec4
        data.color = glm::mix(p.colorBegin, p.colorEnd, lifeT);

        // Explicitly convert float -> vec4 (put size in x)
        float currentSize = glm::mix(p.sizeBegin, p.sizeEnd, lifeT);
        data.size = glm::vec4(currentSize, 0.0f, 0.0f, 0.0f);

        instanceData.push_back(data);
    }

    if (!instanceData.empty()) {
        VkDeviceSize size = instanceData.size() * sizeof(InstanceData);
        void* data;
        vkMapMemory(device, instanceBuffers[currentFrame]->GetBufferMemory(), 0, size, 0, &data);
        memcpy(data, instanceData.data(), (size_t)size);
        vkUnmapMemory(device, instanceBuffers[currentFrame]->GetBufferMemory());
    }
}

void ParticleSystem::Draw(VkCommandBuffer cmd, VkDescriptorSet globalDescriptorSet, uint32_t currentFrame) {
    // Update the GPU buffer for THIS frame right before drawing
    UpdateInstanceBuffer(currentFrame);

    uint32_t activeCount = 0;
    for (const auto& p : particles) if (p.active) activeCount++;

    if (activeCount == 0 || !pipeline) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

    VkDescriptorSet sets[] = { globalDescriptorSet, descriptorSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 2, sets, 0, nullptr);

    VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    // Bind the Instance Buffer for the CURRENT frame
    VkBuffer instanceBufferRaw[] = { instanceBuffers[currentFrame]->GetBuffer() };
    vkCmdBindVertexBuffers(cmd, 1, 1, instanceBufferRaw, offsets);

    vkCmdDraw(cmd, 6, activeCount, 0, 0);
}

void ParticleSystem::SetupBuffers() {
    // x, y, z, u, v
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
    };

    vertexBuffer = std::make_unique<VulkanBuffer>(device, physicalDevice);
    vertexBuffer->CreateBuffer(sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vertexBuffer->CopyData(vertices, sizeof(vertices));

    instanceBuffers.resize(framesInFlight);
    for (size_t i = 0; i < framesInFlight; i++) {
        instanceBuffers[i] = std::make_unique<VulkanBuffer>(device, physicalDevice);
        instanceBuffers[i]->CreateBuffer(maxParticles * sizeof(InstanceData),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

// Static definitions for Pipeline Creation
std::vector<VkVertexInputBindingDescription> ParticleSystem::GetBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindings(2);

    // Binding 0: Mesh Data
    bindings[0].binding = 0;
    bindings[0].stride = 5 * sizeof(float); // x,y,z,u,v
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Binding 1: Instance Data
    bindings[1].binding = 1;
    bindings[1].stride = sizeof(InstanceData);
    bindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    return bindings;
}

std::vector<VkVertexInputAttributeDescription> ParticleSystem::GetAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attribs;

    // Binding 0: Mesh Data (Unchanged)
    // Location 0: Position (vec3)
    attribs.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
    // Location 1: UV (vec2)
    attribs.push_back({ 1, 0, VK_FORMAT_R32G32_SFLOAT, 3 * sizeof(float) });

    // Binding 1: Instance Data (Modified for vec4 alignment)

    // Location 2: Position (Host sends vec4, Shader reads vec3. Uses xyz.)
    attribs.push_back({ 2, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, position) });

    // Location 3: Color (Host sends vec4, Shader reads vec4)
    attribs.push_back({ 3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, color) });

    // Location 4: Size (Host sends vec4, Shader reads float. Uses x.)
    attribs.push_back({ 4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, size) });

    return attribs;
}
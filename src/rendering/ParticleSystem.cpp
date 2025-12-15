#include "ParticleSystem.h"
#include <random>
#include <algorithm> // For std::sort

// Helper for random numbers
static float RandomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(mt);
}

ParticleSystem::ParticleSystem(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, uint32_t maxParticles)
    : device(device), physicalDevice(physicalDevice), maxParticles(maxParticles) {
    particles.resize(maxParticles);
    poolIndex = maxParticles - 1;

    // Create Texture Loader
    texture = std::make_unique<Texture>(device, physicalDevice, commandPool, graphicsQueue);
}

ParticleSystem::~ParticleSystem() {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, textureLayout, nullptr);

    pipeline.reset();
    texture.reset();
    vertexBuffer.reset();
    instanceBuffer.reset();
}

void ParticleSystem::Initialize(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout, const std::string& texturePath, bool additiveBlending) {
    texture->LoadFromFile(texturePath);
    SetupBuffers();

    // Create Layout for texture
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &textureLayout);

    // Pool & Set
    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

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

    SetupPipeline(renderPass, globalSetLayout, additiveBlending);
}

void ParticleSystem::Emit(const ParticleProps& props) {
    Particle& p = particles[poolIndex];
    p.active = true;
    p.position = props.position;

    // Random velocity
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

    poolIndex = (poolIndex - 1) % maxParticles;
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

        // Spawn particles if enough time has passed
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

        // Simple sorting distance (squared) relative to origin for now
        // Real sorting requires camera position passed to Update
        // But for additive blending (Fire), sorting isn't needed!
    }

    UpdateInstanceBuffer();
}

void ParticleSystem::UpdateInstanceBuffer() {
    std::vector<InstanceData> instanceData;
    instanceData.reserve(maxParticles); // Estimation

    for (const auto& p : particles) {
        if (!p.active) continue;

        float lifeT = 1.0f - (p.lifeRemaining / p.lifeTime); // 0 to 1

        InstanceData data;
        data.position = p.position;
        data.color = glm::mix(p.colorBegin, p.colorEnd, lifeT);
        data.size = glm::mix(p.sizeBegin, p.sizeEnd, lifeT);
        instanceData.push_back(data);
    }

    if (!instanceData.empty()) {
        VkDeviceSize size = instanceData.size() * sizeof(InstanceData);
        // Map and Copy. Ideally use a staging buffer for best practice, 
        // but mapping a HOST_VISIBLE buffer directly is fine for dynamic particles.
        void* data;
        vkMapMemory(device, instanceBuffer->GetBufferMemory(), 0, size, 0, &data);
        memcpy(data, instanceData.data(), (size_t)size);
        vkUnmapMemory(device, instanceBuffer->GetBufferMemory());
    }
}

void ParticleSystem::Draw(VkCommandBuffer cmd, VkDescriptorSet globalDescriptorSet) {
    uint32_t activeCount = 0;
    for (const auto& p : particles) if (p.active) activeCount++;

    if (activeCount == 0) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

    // Set 0: Global UBO, Set 1: Particle Texture
    VkDescriptorSet sets[] = { globalDescriptorSet, descriptorSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 2, sets, 0, nullptr);

    VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    // Bind Instance Buffer as binding 1
    VkBuffer instanceBuffers[] = { instanceBuffer->GetBuffer() };
    vkCmdBindVertexBuffers(cmd, 1, 1, instanceBuffers, offsets); // Binding 1

    // Draw 6 vertices (1 quad) * activeCount instances
    vkCmdDraw(cmd, 6, activeCount, 0, 0);
}

void ParticleSystem::SetupBuffers() {
    // 1. Static Quad Vertex Buffer
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

    // 2. Dynamic Instance Buffer
    instanceBuffer = std::make_unique<VulkanBuffer>(device, physicalDevice);
    instanceBuffer->CreateBuffer(maxParticles * sizeof(InstanceData),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void ParticleSystem::SetupPipeline(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout, bool additive) {
    GraphicsPipelineConfig config{};
    config.vertShaderPath = "src/shaders/particle_vert.spv"; // Make sure to compile this!
    config.fragShaderPath = "src/shaders/particle_frag.spv";
    config.renderPass = renderPass;
    config.extent = { 800, 600 }; // Viewport, updated dynamic anyway

    // Binding 0: Mesh Data
    VkVertexInputBindingDescription mainBinding{};
    mainBinding.binding = 0;
    mainBinding.stride = 5 * sizeof(float); // x,y,z,u,v
    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Binding 1: Instance Data
    VkVertexInputBindingDescription instanceBinding{};
    instanceBinding.binding = 1;
    instanceBinding.stride = sizeof(InstanceData);
    instanceBinding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    VkVertexInputBindingDescription bindings[] = { mainBinding, instanceBinding };
    config.bindingDescription = bindings; // Pointer workaround: store in member if needed, or array logic in GraphicsPipeline

    // Attributes
    std::vector<VkVertexInputAttributeDescription> attribs;
    // Binding 0
    attribs.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });    // Pos
    attribs.push_back({ 1, 0, VK_FORMAT_R32G32_SFLOAT, 3 * sizeof(float) }); // UV
    // Binding 1 (Instance)
    attribs.push_back({ 2, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(InstanceData, position) });
    attribs.push_back({ 3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, color) });
    attribs.push_back({ 4, 1, VK_FORMAT_R32_SFLOAT, offsetof(InstanceData, size) });

    // Hack: The GraphicsPipeline class expects pointers. 
    // You might need to adjust GraphicsPipeline to take a vector, or handle this carefully.
    // For now, assuming you update GraphicsPipeline to handle multiple bindings/attributes nicely.

    config.descriptorSetLayouts = { globalSetLayout, textureLayout };

    // Blending
    config.blendEnable = true;
    if (additive) {
        // Fire / Magic
        config.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        config.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    }
    else {
        // Smoke / Dust
        config.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        config.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    }

    config.depthWriteEnable = false; // Important for particles!
    config.depthTestEnable = true;

    config.attributeDescriptions = attribs.data();
    config.attributeCount = static_cast<uint32_t>(attribs.size());

    config.bindingCount = 2; // Binding 0 (Quad) + Binding 1 (Instance Data)

    // 3. Create the Pipeline
    pipeline = std::make_unique<GraphicsPipeline>(device, config);
    pipeline->Create();
}


#pragma once
#include "ParticleSystem.h"
#include <glm/glm.hpp>

namespace ParticleLibrary {

    inline ParticleProps GetFireProps() {
        ParticleProps props;
        props.position = glm::vec3(0.0f);
        props.velocity = glm::vec3(0.0f, 2.0f, 0.0f);
        props.velocityVariation = glm::vec3(0.5f, 1.0f, 0.5f);
        props.colorBegin = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
        props.colorEnd = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        props.sizeBegin = 0.5f;
        props.sizeEnd = 0.1f;
        props.sizeVariation = 0.3f;
        props.lifeTime = 1.0f;

        props.texturePath = "textures/kenney_particle-pack/transparent/fire_01.png";
        props.isAdditive = true;
        return props;
    }

    inline ParticleProps GetSmokeProps() {
        ParticleProps props;
        props.position = glm::vec3(0.0f);
        props.velocity = glm::vec3(0.0f, 1.5f, 0.0f);
        props.velocityVariation = glm::vec3(0.8f, 0.5f, 0.8f);
        props.colorBegin = glm::vec4(0.2f, 0.2f, 0.2f, 0.8f);
        props.colorEnd = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        props.sizeBegin = 0.2f;
        props.sizeEnd = 1.5f;
        props.sizeVariation = 0.5f;
        props.lifeTime = 3.0f;

        props.texturePath = "textures/kenney_particle-pack/transparent/smoke_01.png";
        props.isAdditive = false;
        return props;
    }

    inline ParticleProps GetRainProps() {
        ParticleProps props;
        props.velocity = glm::vec3(0.0f, -25.0f, 0.0f);
        props.velocityVariation = glm::vec3(0.2f, 2.0f, 0.2f);
        props.colorBegin = glm::vec4(0.7f, 0.8f, 1.0f, 0.8f);
        props.colorEnd = glm::vec4(0.7f, 0.8f, 1.0f, 0.6f);
        props.sizeBegin = 0.15f;
        props.sizeEnd = 0.15f;
        props.sizeVariation = 0.05f;
        props.lifeTime = 2.0f;

        props.texturePath = "textures/kenney_particle-pack/transparent/circle_05.png"; // Or a specialized drop texture
        props.isAdditive = true;
        return props;
    }

    inline ParticleProps GetSnowProps() {
        ParticleProps props;
        // CHANGE: Much slower fall speed
        props.velocity = glm::vec3(0.0f, -0.5f, 0.0f);
        // Drifting (Velocity Variation) - reduced Y variation to keep them falling down
        props.velocityVariation = glm::vec3(1.5f, 0.2f, 1.5f);

        props.colorBegin = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f);
        props.colorEnd = glm::vec4(1.0f, 1.0f, 1.0f, 0.4f);

        // CHANGE: Much larger size
        props.sizeBegin = 0.6f;
        props.sizeEnd = 0.6f;
        props.sizeVariation = 0.2f;

        props.lifeTime = 12.0f; // Longer life for slower fall

        props.texturePath = "textures/kenney_particle-pack/transparent/star_01.png";
        props.isAdditive = true;
        return props;
    }

    inline ParticleProps GetDustProps() {
        ParticleProps props;
        props.velocity = glm::vec3(0.5f, 0.1f, 0.5f); // Slight drift
        props.velocityVariation = glm::vec3(1.0f, 0.2f, 1.0f);
        props.colorBegin = glm::vec4(0.8f, 0.7f, 0.5f, 0.4f); // Sandy color
        props.colorEnd = glm::vec4(0.8f, 0.7f, 0.5f, 0.0f);
        props.sizeBegin = 0.05f;
        props.sizeEnd = 0.05f;
        props.sizeVariation = 0.02f;
        props.lifeTime = 5.0f;

        props.texturePath = "textures/kenney_particle-pack/transparent/circle_02.png"; // Soft dust
        props.isAdditive = false;
        return props;
    }
}
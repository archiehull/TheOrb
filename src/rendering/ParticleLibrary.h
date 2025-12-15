#pragma once
#include "ParticleSystem.h" // Needed for ParticleProps
#include <glm/glm.hpp>

namespace ParticleLibrary {

    inline ParticleProps GetFireProps() {
        ParticleProps props;
        props.position = glm::vec3(0.0f); // Position is overridden by the emitter usually
        props.velocity = glm::vec3(0.0f, 2.0f, 0.0f);
        props.velocityVariation = glm::vec3(0.5f, 1.0f, 0.5f);
        props.colorBegin = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
        props.colorEnd = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);   // Fade to Red/Transparent
        props.sizeBegin = 0.5f;
        props.sizeEnd = 0.1f;
        props.lifeTime = 1.0f;
        return props;
    }

    inline ParticleProps GetSmokeProps() {
        ParticleProps props;
        props.position = glm::vec3(0.0f);
        props.velocity = glm::vec3(0.0f, 1.5f, 0.0f); // Slower up
        props.velocityVariation = glm::vec3(0.8f, 0.5f, 0.8f); // Wide spread
        props.colorBegin = glm::vec4(0.2f, 0.2f, 0.2f, 0.8f); // Dark Grey
        props.colorEnd = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);   // Fade to Black/Transparent
        props.sizeBegin = 0.2f;
        props.sizeEnd = 1.5f; // Grows larger
        props.lifeTime = 3.0f;
        return props;
    }

    inline ParticleProps GetRainProps() {
        ParticleProps props;
        props.velocity = glm::vec3(0.0f, -20.0f, 0.0f); // Fast down
        props.velocityVariation = glm::vec3(0.1f, 2.0f, 0.1f);
        props.colorBegin = glm::vec4(0.7f, 0.8f, 1.0f, 0.6f); // Light Blue
        props.colorEnd = glm::vec4(0.7f, 0.8f, 1.0f, 0.6f);   // Constant color
        props.sizeBegin = 0.1f;
        props.sizeEnd = 0.1f;
        props.lifeTime = 2.0f;
        return props;
    }
}
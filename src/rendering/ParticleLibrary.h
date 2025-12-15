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
        props.sizeVariation = 0.3f; // <--- ADD THIS (was uninitialized garbage!)
        props.lifeTime = 1.0f;
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
        props.sizeVariation = 0.5f; // <--- ADD THIS
        props.lifeTime = 3.0f;
        return props;
    }

    inline ParticleProps GetRainProps() {
        ParticleProps props;
        props.velocity = glm::vec3(0.0f, -20.0f, 0.0f);
        props.velocityVariation = glm::vec3(0.1f, 2.0f, 0.1f);
        props.colorBegin = glm::vec4(0.7f, 0.8f, 1.0f, 0.6f);
        props.colorEnd = glm::vec4(0.7f, 0.8f, 1.0f, 0.6f);
        props.sizeBegin = 0.1f;
        props.sizeEnd = 0.1f;
        props.sizeVariation = 0.0f; // <--- ADD THIS
        props.lifeTime = 2.0f;
        return props;
    }
}
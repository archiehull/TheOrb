#pragma once
#include "ParticleSystem.h"
#include <glm/glm.hpp>

namespace ParticleLibrary {

    namespace detail {
        inline ParticleProps MakeDefaultProps() {
            ParticleProps p;
            // Keep defaults from ParticleProps where appropriate.
            return p;
        }
    } // namespace detail

    inline const ParticleProps& GetFireProps() {
        static const ParticleProps props = []() {
            ParticleProps p = detail::MakeDefaultProps();
            p.velocity = glm::vec3(0.0f, 2.0f, 0.0f);
            p.velocityVariation = glm::vec3(0.5f, 1.0f, 0.5f);
            p.colorBegin = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
            p.colorEnd = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            p.sizeBegin = 0.5f;
            p.sizeEnd = 0.1f;
            p.sizeVariation = 0.3f;
            p.lifeTime = 1.0f;
            p.texturePath = "textures/kenney_particle-pack/transparent/fire_01.png";
            p.isAdditive = true;
            return p;
            }();
        return props;
    }

    inline const ParticleProps& GetSmokeProps() {
        static const ParticleProps props = []() {
            ParticleProps p = detail::MakeDefaultProps();
            p.velocity = glm::vec3(0.0f, 1.5f, 0.0f);
            p.velocityVariation = glm::vec3(0.8f, 0.5f, 0.8f);
            p.colorBegin = glm::vec4(0.2f, 0.2f, 0.2f, 0.8f);
            p.colorEnd = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            p.sizeBegin = 0.2f;
            p.sizeEnd = 1.5f;
            p.sizeVariation = 0.5f;
            p.lifeTime = 3.0f;
            p.texturePath = "textures/kenney_particle-pack/transparent/smoke_01.png";
            p.isAdditive = false;
            return p;
            }();
        return props;
    }

    inline const ParticleProps& GetRainProps() {
        static const ParticleProps props = []() {
            ParticleProps p = detail::MakeDefaultProps();
            p.velocity = glm::vec3(0.0f, -25.0f, 0.0f);
            p.velocityVariation = glm::vec3(0.2f, 2.0f, 0.2f);
            p.colorBegin = glm::vec4(0.7f, 0.8f, 1.0f, 0.8f);
            p.colorEnd = glm::vec4(0.7f, 0.8f, 1.0f, 0.6f);
            p.sizeBegin = 0.15f;
            p.sizeEnd = 0.15f;
            p.sizeVariation = 0.05f;
            p.lifeTime = 2.0f;
            p.texturePath = "textures/kenney_particle-pack/transparent/circle_05.png";
            p.isAdditive = true;
            return p;
            }();
        return props;
    }

    inline const ParticleProps& GetSnowProps() {
        static const ParticleProps props = []() {
            ParticleProps p = detail::MakeDefaultProps();
            p.velocity = glm::vec3(0.0f, -0.5f, 0.0f);
            p.velocityVariation = glm::vec3(1.5f, 0.2f, 1.5f);
            p.colorBegin = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f);
            p.colorEnd = glm::vec4(1.0f, 1.0f, 1.0f, 0.4f);
            p.sizeBegin = 0.6f;
            p.sizeEnd = 0.6f;
            p.sizeVariation = 0.2f;
            p.lifeTime = 12.0f;
            p.texturePath = "textures/kenney_particle-pack/transparent/star_01.png";
            p.isAdditive = true;
            return p;
            }();
        return props;
    }

    inline const ParticleProps& GetDustProps() {
        static const ParticleProps props = []() {
            ParticleProps p = detail::MakeDefaultProps();
            p.velocity = glm::vec3(0.5f, 0.1f, 0.5f);
            p.velocityVariation = glm::vec3(1.0f, 0.2f, 1.0f);
            p.colorBegin = glm::vec4(0.8f, 0.7f, 0.5f, 0.4f);
            p.colorEnd = glm::vec4(0.8f, 0.7f, 0.5f, 0.0f);
            p.sizeBegin = 0.05f;
            p.sizeEnd = 0.05f;
            p.sizeVariation = 0.02f;
            p.lifeTime = 5.0f;
            p.texturePath = "textures/kenney_particle-pack/transparent/circle_02.png";
            p.isAdditive = false;
            return p;
            }();
        return props;
    }

} // namespace ParticleLibrary
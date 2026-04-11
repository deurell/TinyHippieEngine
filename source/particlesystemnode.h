#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "scenenode.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <random>
#include <vector>

class ParticleSystemNode : public DL::SceneNode {
public:
  enum class EmissionPattern { Random, Spokes };
  enum class EmissionMode { Burst, Continuous };

  struct Config {
    struct Emission {
      int maxParticles = 128;
      int burstCount = 64;
      float rate = 32.0f;
      EmissionMode mode = EmissionMode::Burst;
      EmissionPattern pattern = EmissionPattern::Random;
      int spokes = 16;
      float spread = 0.35f;
      float spawnRadius = 0.0f;
      float angleJitter = glm::radians(4.0f);
      float upwardBias = 0.18f;
      glm::vec3 direction{0.0f, 1.0f, 0.0f};
      float coneAngle = glm::radians(25.0f);
    } emission;

    struct Motion {
      glm::vec3 gravity{0.0f, -32.0f, 0.0f};
      float drag = 0.35f;
      float speedMin = 14.0f;
      float speedMax = 26.0f;
      float angularSpeedMin = glm::radians(-360.0f);
      float angularSpeedMax = glm::radians(360.0f);
    } motion;

    struct Life {
      float min = 0.9f;
      float max = 1.6f;
    } life;

    struct Appearance {
      glm::vec3 startSize{0.25f, 0.25f, 0.25f};
      glm::vec3 endSize{0.02f, 0.02f, 0.02f};
      glm::vec4 startColorMin{0.75f, 0.35f, 0.05f, 1.0f};
      glm::vec4 startColorMax{1.0f, 0.85f, 0.3f, 1.0f};
      glm::vec4 endColor{0.3f, 0.08f, 0.02f, 1.0f};
    } appearance;

    struct RenderStyle {
      float paletteSteps = 4.0f;
      float coreRadius = 0.22f;
      float haloRadius = 0.55f;
      float outerRadius = 1.0f;
      float sparkle = 0.18f;
      glm::vec4 hotColor{1.0f, 0.95f, 0.75f, 1.0f};
      glm::vec4 deepColor{0.9f, 0.35f, 0.08f, 1.0f};
    } render;

    static Config softGlowBurst();
  };

  struct ParticleState {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{0.0f};
    glm::vec4 color{1.0f};
    glm::vec3 rotationAxis{0.0f, 1.0f, 0.0f};
    float angularSpeed = 0.0f;
    float age = 0.0f;
    float lifetime = 1.0f;
    bool alive = false;
    glm::vec4 startColor{1.0f};
  };

  explicit ParticleSystemNode(DL::IRenderDevice *renderDevice,
                              DL::Camera *camera,
                              DL::SceneNode *parentNode = nullptr);
  ParticleSystemNode(DL::IRenderDevice *renderDevice, DL::Camera *camera,
                     Config config, DL::SceneNode *parentNode);
  ~ParticleSystemNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;

  void resetParticles();
  void explode(const glm::vec3 &worldPosition);
  void startEmitting();
  void stopEmitting();
  void setEmitterPosition(const glm::vec3 &position);

  const std::vector<ParticleState> &getParticles() const { return particles_; }
  const Config &getConfig() const { return config_; }

private:
  ParticleState *findInactiveParticle();
  void spawnParticle(ParticleState &particle, std::size_t emissionIndex,
                     const glm::vec3 &origin);

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::Camera *camera_ = nullptr;
  Config config_;
  std::mt19937 twister_;
  glm::vec3 emitterPosition_{0.0f, 0.0f, 0.0f};
  float spawnAccumulator_ = 0.0f;
  bool emitting_ = false;
  std::vector<ParticleState> particles_;
};

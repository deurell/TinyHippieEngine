#include "particlesystemnode.h"

#include "particlevisualizer.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

namespace {

glm::vec3 randomDirection(std::mt19937 &twister) {
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

  glm::vec3 direction(0.0f);
  do {
    direction = {dist(twister), dist(twister), dist(twister) * 0.35f};
  } while (glm::length2(direction) < 0.0001f);

  return glm::normalize(direction);
}

glm::vec3 randomBurstDirection(std::mt19937 &twister, float zScale) {
  auto direction = randomDirection(twister);
  direction.z *= zScale;
  if (glm::length2(direction) < 0.0001f) {
    return {0.0f, 1.0f, 0.0f};
  }
  return glm::normalize(direction);
}

float randomRange(std::mt19937 &twister, float min, float max) {
  std::uniform_real_distribution<float> dist(min, max);
  return dist(twister);
}

glm::vec3 patternedBurstDirection(std::mt19937 &twister, int index,
                                  const ParticleSystemNode::Config &config) {
  const int spokeCount = std::max(1, config.emission.spokes);
  const float angleStep =
      glm::two_pi<float>() / static_cast<float>(spokeCount);
  const int spokeIndex = index % spokeCount;
  const float baseAngle = static_cast<float>(spokeIndex) * angleStep;
  const float angle =
      baseAngle +
      randomRange(twister, -config.emission.angleJitter,
                  config.emission.angleJitter);
  const float ringScale = (index / spokeCount) % 2 == 0 ? 1.0f : 0.74f;

  glm::vec3 direction = {
      std::cos(angle) * ringScale,
      std::sin(angle) * ringScale + config.emission.upwardBias,
      std::sin(angle * 2.0f) * config.emission.spread,
  };

  if (glm::length2(direction) < 0.0001f) {
    return {0.0f, 1.0f, 0.0f};
  }
  return glm::normalize(direction);
}

glm::vec4 randomColor(std::mt19937 &twister, const glm::vec4 &minColor,
                      const glm::vec4 &maxColor) {
  return {
      randomRange(twister, minColor.r, maxColor.r),
      randomRange(twister, minColor.g, maxColor.g),
      randomRange(twister, minColor.b, maxColor.b),
      randomRange(twister, minColor.a, maxColor.a),
  };
}

} // namespace

ParticleSystemNode::ParticleSystemNode(DL::IRenderDevice *renderDevice,
                                       DL::Camera *camera,
                                       DL::SceneNode *parentNode)
    : ParticleSystemNode(renderDevice, camera, Config{}, parentNode) {}

ParticleSystemNode::ParticleSystemNode(DL::IRenderDevice *renderDevice,
                                       DL::Camera *camera,
                                       Config config,
                                       DL::SceneNode *parentNode)
    : DL::SceneNode(parentNode), renderDevice_(renderDevice), camera_(camera),
      config_(config) {
  std::random_device device;
  twister_ = std::mt19937(device());
}

ParticleSystemNode::Config ParticleSystemNode::Config::softGlowBurst() {
  Config config;
  config.emission.count = 128;
  config.emission.pattern = EmissionPattern::Random;
  config.emission.spokes = 10;
  config.emission.spread = 0.18f;
  config.emission.angleJitter = glm::radians(1.0f);
  config.emission.upwardBias = 0.05f;

  config.motion.gravity = {0.0f, -13.0f, 0.0f};
  config.motion.drag = 0.18f;
  config.motion.speedMin = 8.5f;
  config.motion.speedMax = 13.0f;
  config.motion.angularSpeedMin = glm::radians(-120.0f);
  config.motion.angularSpeedMax = glm::radians(120.0f);

  config.life.min = 1.1f;
  config.life.max = 1.5f;

  config.appearance.startSize = {0.28f, 0.28f, 0.28f};
  config.appearance.endSize = {0.1f, 0.1f, 0.1f};
  config.appearance.startColorMin = {0.9f, 0.6f, 0.22f, 1.0f};
  config.appearance.startColorMax = {1.0f, 0.88f, 0.52f, 1.0f};
  config.appearance.endColor = {0.26f, 0.09f, 0.02f, 1.0f};

  config.render.paletteSteps = 2.0f;
  config.render.coreRadius = 0.14f;
  config.render.haloRadius = 0.5f;
  config.render.outerRadius = 0.9f;
  config.render.sparkle = 0.02f;
  config.render.hotColor = {1.0f, 0.92f, 0.68f, 1.0f};
  config.render.deepColor = {0.88f, 0.3f, 0.04f, 1.0f};
  return config;
}

void ParticleSystemNode::init() {
  SceneNode::init();

  auto visualizer = std::make_unique<DL::ParticleVisualizer>(
      "ParticleVisualizer", *camera_, *this, renderDevice_);
  visualizers.emplace_back(std::move(visualizer));

  particles_.reserve(static_cast<std::size_t>(config_.emission.count));

  for (int i = 0; i < config_.emission.count; ++i) {
    ParticleState state;
    state.rotationAxis = randomDirection(twister_);
    state.angularSpeed = randomRange(twister_, config_.motion.angularSpeedMin,
                                     config_.motion.angularSpeedMax);
    state.scale = {0.0f, 0.0f, 0.0f};
    state.color = config_.appearance.endColor;
    particles_.push_back(state);
  }

  resetParticles();
}

void ParticleSystemNode::update(const DL::FrameContext &ctx) {
  for (auto &particle : particles_) {
    if (!particle.alive) {
      continue;
    }

    particle.age += ctx.delta_time;
    if (particle.age >= particle.lifetime) {
      particle.alive = false;
      particle.velocity = {0.0f, 0.0f, 0.0f};
      particle.scale = {0.0f, 0.0f, 0.0f};
      particle.color = config_.appearance.endColor;
      continue;
    }

    particle.velocity += config_.motion.gravity * ctx.delta_time;
    particle.velocity *=
        std::max(0.0f, 1.0f - config_.motion.drag * ctx.delta_time);
    particle.position += particle.velocity * ctx.delta_time;

    const float t = particle.age / particle.lifetime;
    particle.scale =
        glm::mix(config_.appearance.startSize, config_.appearance.endSize, t);
    particle.color =
        glm::mix(particle.startColor, config_.appearance.endColor, t);

    const auto angle = static_cast<float>(ctx.total_time) * particle.angularSpeed;
    particle.rotation = glm::angleAxis(angle, particle.rotationAxis);
  }

  SceneNode::update(ctx);
}

void ParticleSystemNode::resetParticles() {
  for (auto &particle : particles_) {
    particle.alive = false;
    particle.age = 0.0f;
    particle.lifetime = config_.life.max;
    particle.position = {0.0f, 0.0f, 0.0f};
    particle.velocity = {0.0f, 0.0f, 0.0f};
    particle.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    particle.scale = {0.0f, 0.0f, 0.0f};
    particle.color = config_.appearance.endColor;
  }
}

void ParticleSystemNode::explode(const glm::vec3 &worldPosition) {
  for (std::size_t i = 0; i < particles_.size(); ++i) {
    auto &particle = particles_[i];
    particle.alive = true;
    particle.age = 0.0f;
    particle.lifetime = randomRange(twister_, config_.life.min, config_.life.max);
    particle.startColor =
        randomColor(twister_, config_.appearance.startColorMin,
                    config_.appearance.startColorMax);
    particle.position = worldPosition;
    particle.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    particle.scale = config_.appearance.startSize;
    particle.color = particle.startColor;
    const glm::vec3 direction =
        config_.emission.pattern == EmissionPattern::Spokes
            ? patternedBurstDirection(twister_, static_cast<int>(i), config_)
            : randomBurstDirection(twister_, config_.emission.spread);
    const float ringSpeed =
        config_.emission.pattern == EmissionPattern::Spokes &&
                (i / static_cast<std::size_t>(std::max(1, config_.emission.spokes))) % 2 != 0
            ? 0.82f
            : 1.0f;
    particle.velocity = direction *
                        randomRange(twister_, config_.motion.speedMin,
                                    config_.motion.speedMax) *
                        ringSpeed;
  }
}

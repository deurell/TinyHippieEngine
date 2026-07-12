#include "particlevisualizer.h"
#include "particlesystemnode.h"

#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>
#include <gtest/gtest.h>

TEST(ParticleSystemNodeTest, WaterFountainPresetIsVolumetricForSampleScene) {
  const ParticleSystemNode::Config config =
      ParticleSystemNode::Config::waterFountain();

  EXPECT_EQ(config.emission.mode,
            ParticleSystemNode::EmissionMode::Continuous);
  EXPECT_EQ(config.emission.pattern, ParticleSystemNode::EmissionPattern::Random);
  EXPECT_GE(config.emission.spread, 0.5f);
  EXPECT_GE(config.emission.spawnRadius, 0.25f);
  EXPECT_GE(config.emission.coneAngle, glm::radians(18.0f));
  EXPECT_LE(config.life.max, 1.2f);
  EXPECT_LT(config.motion.speedMax / -config.motion.gravity.y,
            config.life.max);
  EXPECT_GT(config.render.sparkle, 0.0f);
}

TEST(ParticleSystemNodeTest, BillboardModelFacesParticleTowardCameraPosition) {
  DL::Camera camera(glm::vec3(4.0f, 2.0f, 3.0f));
  camera.lookAt(glm::vec3(0.0f));

  const glm::vec3 particlePosition{1.0f, 0.5f, -0.25f};
  const glm::vec3 scale{0.4f, 0.6f, 0.8f};
  const glm::mat4 model =
      DL::ParticleVisualizer::buildBillboardModel(particlePosition, scale,
                                                  camera);

  const glm::vec3 modelForward = glm::normalize(glm::vec3(model[2]));
  const glm::vec3 cameraDirection =
      glm::normalize(camera.getPosition() - particlePosition);

  EXPECT_TRUE(glm::all(glm::epsilonEqual(glm::vec3(model[3]), particlePosition,
                                         0.0001f)));
  EXPECT_TRUE(glm::epsilonEqual(glm::dot(modelForward, cameraDirection), 1.0f,
                                0.0001f));
  EXPECT_TRUE(glm::epsilonEqual(glm::length(glm::vec3(model[0])), scale.x,
                                0.0001f));
  EXPECT_TRUE(glm::epsilonEqual(glm::length(glm::vec3(model[1])), scale.y,
                                0.0001f));
}

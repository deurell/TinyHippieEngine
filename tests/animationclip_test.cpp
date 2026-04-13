#include "animationclip.h"
#include <gtest/gtest.h>
#include <glm/geometric.hpp>

namespace {

TEST(AnimationClipTest, EvaluatesLinearTranslationSampler) {
  DL::AnimationClip clip;
  DL::AnimationSampler sampler;
  sampler.times = {0.0f, 1.0f};
  sampler.values = {glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
                    glm::vec4(10.0f, 0.0f, 0.0f, 0.0f)};
  clip.samplers.push_back(sampler);
  clip.channels.push_back(
      {.nodeIndex = 0,
       .path = DL::AnimationTargetPath::Translation,
       .samplerIndex = 0});
  clip.duration = 1.0f;

  auto pose = DL::makeAnimationPose(1);
  DL::evaluateAnimationClip(clip, 0.5f, pose);

  ASSERT_EQ(pose.nodes.size(), 1u);
  EXPECT_TRUE(pose.nodes[0].hasTranslation);
  EXPECT_FLOAT_EQ(pose.nodes[0].translation.x, 5.0f);
}

TEST(AnimationClipTest, EvaluatesRotationWithSlerp) {
  DL::AnimationClip clip;
  DL::AnimationSampler sampler;
  sampler.times = {0.0f, 1.0f};
  sampler.values = {
      glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
      glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
  };
  clip.samplers.push_back(sampler);
  clip.channels.push_back(
      {.nodeIndex = 0,
       .path = DL::AnimationTargetPath::Rotation,
       .samplerIndex = 0});
  clip.duration = 1.0f;

  auto pose = DL::makeAnimationPose(1);
  DL::evaluateAnimationClip(clip, 0.5f, pose);

  ASSERT_EQ(pose.nodes.size(), 1u);
  EXPECT_TRUE(pose.nodes[0].hasRotation);
  EXPECT_NEAR(glm::length(pose.nodes[0].rotation), 1.0f, 1e-5f);
}

} // namespace

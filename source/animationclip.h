#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace DL {

enum class AnimationInterpolation {
  Linear,
  Step,
  CubicSpline,
};

enum class AnimationTargetPath {
  Translation,
  Rotation,
  Scale,
  Weights,
};

struct AnimationSampler {
  std::vector<float> times;
  std::vector<glm::vec4> values;
  AnimationInterpolation interpolation = AnimationInterpolation::Linear;
};

struct AnimationChannel {
  int nodeIndex = -1;
  AnimationTargetPath path = AnimationTargetPath::Translation;
  std::size_t samplerIndex = 0;
};

struct AnimationClip {
  std::string name;
  std::vector<AnimationSampler> samplers;
  std::vector<AnimationChannel> channels;
  float duration = 0.0f;
};

struct AnimatedNodeTransform {
  bool hasTranslation = false;
  glm::vec3 translation{0.0f, 0.0f, 0.0f};
  bool hasRotation = false;
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
  bool hasScale = false;
  glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

struct AnimationPose {
  std::vector<AnimatedNodeTransform> nodes;
};

AnimationPose makeAnimationPose(std::size_t nodeCount);
void evaluateAnimationClip(const AnimationClip &clip, float time,
                           AnimationPose &pose);

} // namespace DL

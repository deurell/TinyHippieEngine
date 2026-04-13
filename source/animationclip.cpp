#include "animationclip.h"

#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_geometric.hpp>

namespace DL {
namespace {

float wrapTime(float time, float duration) {
  if (duration <= 0.0f) {
    return 0.0f;
  }
  const float wrapped = std::fmod(time, duration);
  return wrapped < 0.0f ? wrapped + duration : wrapped;
}

glm::vec4 sampleValue(const AnimationSampler &sampler, float time) {
  if (sampler.times.empty() || sampler.values.empty()) {
    return glm::vec4(0.0f);
  }

  if (sampler.times.size() == 1 || sampler.values.size() == 1) {
    return sampler.values.front();
  }

  const float localTime = wrapTime(time, sampler.times.back());
  const auto upper = std::upper_bound(sampler.times.begin(), sampler.times.end(),
                                      localTime);
  if (upper == sampler.times.begin()) {
    return sampler.values.front();
  }
  if (upper == sampler.times.end()) {
    return sampler.values.back();
  }

  const std::size_t nextIndex =
      static_cast<std::size_t>(upper - sampler.times.begin());
  const std::size_t prevIndex = nextIndex - 1;
  if (sampler.interpolation == AnimationInterpolation::Step) {
    return sampler.values[prevIndex];
  }

  const float startTime = sampler.times[prevIndex];
  const float endTime = sampler.times[nextIndex];
  if (endTime <= startTime) {
    return sampler.values[prevIndex];
  }

  const float alpha = (localTime - startTime) / (endTime - startTime);
  return glm::mix(sampler.values[prevIndex], sampler.values[nextIndex], alpha);
}

glm::quat sampleRotation(const AnimationSampler &sampler, float time) {
  if (sampler.times.empty() || sampler.values.empty()) {
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  }

  if (sampler.times.size() == 1 || sampler.values.size() == 1) {
    return glm::normalize(glm::quat(sampler.values.front().w,
                                    sampler.values.front().x,
                                    sampler.values.front().y,
                                    sampler.values.front().z));
  }

  const float localTime = wrapTime(time, sampler.times.back());
  const auto upper = std::upper_bound(sampler.times.begin(), sampler.times.end(),
                                      localTime);
  if (upper == sampler.times.begin()) {
    const auto &value = sampler.values.front();
    return glm::normalize(glm::quat(value.w, value.x, value.y, value.z));
  }
  if (upper == sampler.times.end()) {
    const auto &value = sampler.values.back();
    return glm::normalize(glm::quat(value.w, value.x, value.y, value.z));
  }

  const std::size_t nextIndex =
      static_cast<std::size_t>(upper - sampler.times.begin());
  const std::size_t prevIndex = nextIndex - 1;
  const auto &prevValue = sampler.values[prevIndex];
  const auto &nextValue = sampler.values[nextIndex];
  const glm::quat prevRotation =
      glm::normalize(glm::quat(prevValue.w, prevValue.x, prevValue.y, prevValue.z));
  const glm::quat nextRotation =
      glm::normalize(glm::quat(nextValue.w, nextValue.x, nextValue.y, nextValue.z));

  if (sampler.interpolation == AnimationInterpolation::Step) {
    return prevRotation;
  }

  const float startTime = sampler.times[prevIndex];
  const float endTime = sampler.times[nextIndex];
  if (endTime <= startTime) {
    return prevRotation;
  }

  const float alpha = (localTime - startTime) / (endTime - startTime);
  return glm::normalize(glm::slerp(prevRotation, nextRotation, alpha));
}

} // namespace

AnimationPose makeAnimationPose(std::size_t nodeCount) {
  AnimationPose pose;
  pose.nodes.resize(nodeCount);
  return pose;
}

void evaluateAnimationClip(const AnimationClip &clip, float time,
                           AnimationPose &pose) {
  for (auto &node : pose.nodes) {
    node = AnimatedNodeTransform{};
  }

  for (const auto &channel : clip.channels) {
    if (channel.nodeIndex < 0 ||
        static_cast<std::size_t>(channel.nodeIndex) >= pose.nodes.size() ||
        channel.samplerIndex >= clip.samplers.size()) {
      continue;
    }

    const auto &sampler = clip.samplers[channel.samplerIndex];
    auto &node = pose.nodes[static_cast<std::size_t>(channel.nodeIndex)];
    switch (channel.path) {
    case AnimationTargetPath::Translation: {
      const glm::vec4 value = sampleValue(sampler, time);
      node.hasTranslation = true;
      node.translation = {value.x, value.y, value.z};
      break;
    }
    case AnimationTargetPath::Rotation:
      node.hasRotation = true;
      node.rotation = sampleRotation(sampler, time);
      break;
    case AnimationTargetPath::Scale: {
      const glm::vec4 value = sampleValue(sampler, time);
      node.hasScale = true;
      node.scale = {value.x, value.y, value.z};
      break;
    }
    case AnimationTargetPath::Weights:
      break;
    }
  }
}

} // namespace DL

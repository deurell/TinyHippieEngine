#include "animationplayer.h"

#include <algorithm>
#include <cmath>

namespace DL {

void AnimationPlayer::update(const std::vector<AnimationClip> &clips,
                             float deltaTime) {
  if (!playing_ || clips.empty() || clipIndex_ >= clips.size()) {
    return;
  }

  const float duration = clips[clipIndex_].duration;
  if (duration <= 0.0f) {
    time_ = 0.0f;
    return;
  }

  time_ += deltaTime * playbackSpeed_;
  if (looping_) {
    time_ = std::fmod(time_, duration);
    if (time_ < 0.0f) {
      time_ += duration;
    }
  } else {
    time_ = std::clamp(time_, 0.0f, duration);
    if (time_ >= duration) {
      playing_ = false;
    }
  }
}

void AnimationPlayer::setClipIndex(std::size_t index, bool resetTime) {
  clipIndex_ = index;
  if (resetTime) {
    time_ = 0.0f;
  }
}

} // namespace DL

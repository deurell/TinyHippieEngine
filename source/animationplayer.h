#pragma once

#include "animationclip.h"
#include <cstddef>
#include <vector>

namespace DL {

class AnimationPlayer {
public:
  void update(const std::vector<AnimationClip> &clips, float deltaTime);

  void setPlaying(bool playing) { playing_ = playing; }
  [[nodiscard]] bool isPlaying() const { return playing_; }

  void setLooping(bool looping) { looping_ = looping; }
  [[nodiscard]] bool isLooping() const { return looping_; }

  void setPlaybackSpeed(float speed) { playbackSpeed_ = speed; }
  [[nodiscard]] float playbackSpeed() const { return playbackSpeed_; }

  void setClipIndex(std::size_t index, bool resetTime = true);
  [[nodiscard]] std::size_t clipIndex() const { return clipIndex_; }

  void setTime(float time) { time_ = time; }
  [[nodiscard]] float time() const { return time_; }

private:
  bool playing_ = true;
  bool looping_ = true;
  float playbackSpeed_ = 1.0f;
  std::size_t clipIndex_ = 0;
  float time_ = 0.0f;
};

} // namespace DL

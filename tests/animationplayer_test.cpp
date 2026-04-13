#include "animationplayer.h"
#include <gtest/gtest.h>

namespace {

DL::AnimationClip makeClip(float duration) {
  DL::AnimationClip clip;
  clip.duration = duration;
  return clip;
}

TEST(AnimationPlayerTest, AdvancesTimeWhilePlaying) {
  DL::AnimationPlayer player;
  const std::vector<DL::AnimationClip> clips = {makeClip(2.0f)};

  player.update(clips, 0.5f);

  EXPECT_FLOAT_EQ(player.time(), 0.5f);
}

TEST(AnimationPlayerTest, WrapsTimeWhenLooping) {
  DL::AnimationPlayer player;
  const std::vector<DL::AnimationClip> clips = {makeClip(1.0f)};

  player.update(clips, 1.5f);

  EXPECT_FLOAT_EQ(player.time(), 0.5f);
}

TEST(AnimationPlayerTest, StopsAtEndWhenNotLooping) {
  DL::AnimationPlayer player;
  const std::vector<DL::AnimationClip> clips = {makeClip(1.0f)};
  player.setLooping(false);

  player.update(clips, 2.0f);

  EXPECT_FLOAT_EQ(player.time(), 1.0f);
  EXPECT_FALSE(player.isPlaying());
}

} // namespace

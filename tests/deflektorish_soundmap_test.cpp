#include "game/deflektorish/deflektorishsoundmap.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>

namespace {

TEST(DeflektorishSoundMapTest, ParsesSoundEventData) {
  constexpr char kSource[] = R"json({
    "audioRoot": "Audio",
    "events": {
      "target_first_hit": {
        "clip": "impact.ogg",
        "volume": 0.25,
        "volumePerEnergy": 0.05,
        "maxVolume": 0.7,
        "pitchMin": 0.9,
        "pitchMax": 1.1,
        "poolSize": 6,
        "maxConcurrent": 3
      },
      "target_destroyed": {
        "clip": "boom.ogg",
        "volume": 0.8,
        "volumePerEnergy": 0.02,
        "maxVolume": 1.0,
        "pitchMin": 0.95,
        "pitchMax": 1.05,
        "poolSize": 12
      }
    }
  })json";

  const Deflektorish::SoundMapConfig soundMap =
      Deflektorish::parseSoundMap(kSource, "unit");

  ASSERT_NE(soundMap.find(Deflektorish::Sound::TargetFirstHit), nullptr);
  const Deflektorish::SoundEventConfig *firstHit =
      soundMap.find(Deflektorish::Sound::TargetFirstHit);
  EXPECT_EQ(soundMap.audioRoot, "Audio");
  EXPECT_EQ(firstHit->clip, "impact.ogg");
  EXPECT_FLOAT_EQ(firstHit->volume, 0.25f);
  EXPECT_FLOAT_EQ(firstHit->pitchMax, 1.1f);
  EXPECT_EQ(firstHit->poolSize, 6);
  EXPECT_EQ(firstHit->maxConcurrent, 3);
}

TEST(DeflektorishSoundMapTest, LoadsDefaultSoundMapFile) {
  std::filesystem::path path =
      "../Resources/Game/Deflektorish/Audio/sounds.json";
  if (!std::filesystem::exists(path)) {
    path = "Resources/Game/Deflektorish/Audio/sounds.json";
  }

  const Deflektorish::SoundMapConfig soundMap =
      Deflektorish::loadSoundMap(path);

  ASSERT_NE(soundMap.find(Deflektorish::Sound::TargetFirstHit), nullptr);
  ASSERT_NE(soundMap.find(Deflektorish::Sound::TargetDestroyed), nullptr);
  EXPECT_EQ(soundMap.find(Deflektorish::Sound::TargetFirstHit)->clip,
            "impactMetal_001.ogg");
  EXPECT_EQ(soundMap.find(Deflektorish::Sound::TargetDestroyed)->clip,
            "lowFrequency_explosion_001.ogg");
}

TEST(DeflektorishSoundMapTest, RejectsInvalidPitchRange) {
  constexpr char kSource[] = R"json({
    "events": {
      "target_first_hit": {
        "clip": "impact.ogg",
        "pitchMin": 1.2,
        "pitchMax": 0.8
      },
      "target_destroyed": {
        "clip": "boom.ogg"
      }
    }
  })json";

  EXPECT_THROW((void)Deflektorish::parseSoundMap(kSource, "bad"),
               std::runtime_error);
}

} // namespace

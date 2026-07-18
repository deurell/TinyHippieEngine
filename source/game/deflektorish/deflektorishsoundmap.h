#pragma once

#include "game/deflektorish/deflektorishsound.h"

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace Deflektorish {

struct SoundEventConfig {
  std::string clip;
  float volume = 1.0f;
  float volumePerEnergy = 0.0f;
  float maxVolume = 1.0f;
  float pitchMin = 1.0f;
  float pitchMax = 1.0f;
  int poolSize = 8;
  int maxConcurrent = 8;
};

struct SoundMapConfig {
  std::string audioRoot = "Resources/Game/Deflektorish/Audio";
  std::map<Sound, SoundEventConfig> events;

  [[nodiscard]] const SoundEventConfig *find(Sound sound) const;
};

const char *soundEventKey(Sound sound);
SoundMapConfig parseSoundMap(std::string_view source,
                             std::string_view sourceName);
SoundMapConfig loadSoundMap(const std::filesystem::path &path);

} // namespace Deflektorish

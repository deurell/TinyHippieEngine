#pragma once

#include "miniaudio.h"
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace DL {

class AudioSystem {
public:
  using SoundId = std::uint64_t;
  static constexpr SoundId kInvalidSoundId = 0;

  AudioSystem() = default;
  ~AudioSystem();

  AudioSystem(const AudioSystem &) = delete;
  AudioSystem &operator=(const AudioSystem &) = delete;

  bool init();
  void shutdown();
  [[nodiscard]] bool isInitialized() const { return initialized_; }

  bool loadClip(const std::string &name, const std::string &fileName);
  [[nodiscard]] bool hasClip(const std::string &name) const;

  SoundId playOneShot(const std::string &name, float volume = 1.0f);
  SoundId playLoop(const std::string &name, float volume = 1.0f);
  void stop(SoundId id);
  void stopAll();

  void update();

  void setMasterVolume(float volume);
  [[nodiscard]] float masterVolume() const { return masterVolume_; }
  [[nodiscard]] std::size_t loadedClipCount() const { return clips_.size(); }
  [[nodiscard]] std::size_t activeSoundCount() const {
    return activeSounds_.size();
  }

private:
  struct ActiveSound {
    std::unique_ptr<ma_sound> sound;
    bool looping = false;
  };

  SoundId playInternal(const std::string &name, bool loop, float volume);
  void cleanupFinishedSounds();

  ma_engine engine_{};
  bool initialized_ = false;
  float masterVolume_ = 1.0f;
  SoundId nextSoundId_ = 1;
  std::map<std::string, std::string> clips_;
  std::map<SoundId, ActiveSound> activeSounds_;
};

} // namespace DL

#pragma once

#include "miniaudio.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace DL {

enum class AudioGroup : std::uint8_t {
  Music = 0,
  SFX,
  UI,
  Ambience,
  Count
};

class AudioSystem {
public:
  using SoundId = std::uint64_t;
  static constexpr SoundId kInvalidSoundId = 0;
  static constexpr std::size_t kGroupCount =
      static_cast<std::size_t>(AudioGroup::Count);

  AudioSystem() = default;
  ~AudioSystem();

  AudioSystem(const AudioSystem &) = delete;
  AudioSystem &operator=(const AudioSystem &) = delete;

  bool init();
  void shutdown();
  [[nodiscard]] bool isInitialized() const { return initialized_; }

  bool loadClip(const std::string &name, const std::string &fileName);
  [[nodiscard]] bool hasClip(const std::string &name) const;

  SoundId playOneShot(const std::string &name,
                      AudioGroup group = AudioGroup::SFX,
                      float volume = 1.0f);
  SoundId playLoop(const std::string &name,
                   AudioGroup group = AudioGroup::SFX,
                   float volume = 1.0f);
  void stop(SoundId id);
  void stopAll();

  void update();

  void setMasterVolume(float volume);
  void setGroupVolume(AudioGroup group, float volume);
  [[nodiscard]] float groupVolume(AudioGroup group) const;
  void setGroupMuted(AudioGroup group, bool muted);
  [[nodiscard]] bool isGroupMuted(AudioGroup group) const;

  [[nodiscard]] static const char *groupLabel(AudioGroup group);
  [[nodiscard]] float masterVolume() const { return masterVolume_; }
  [[nodiscard]] std::size_t loadedClipCount() const { return clips_.size(); }
  [[nodiscard]] std::size_t activeSoundCount() const {
    return activeSounds_.size();
  }
  [[nodiscard]] std::size_t activeSoundCount(AudioGroup group) const;

private:
  struct GroupState {
    ma_sound_group soundGroup{};
    bool initialized = false;
    float volume = 1.0f;
    bool muted = false;
  };

  struct ActiveSound {
    std::unique_ptr<ma_sound> sound;
    bool looping = false;
    AudioGroup group = AudioGroup::SFX;
  };

  SoundId playInternal(const std::string &name, AudioGroup group, bool loop,
                       float volume);
  bool initGroups();
  void uninitGroups();
  void applyGroupGain(AudioGroup group);
  [[nodiscard]] static std::size_t toGroupIndex(AudioGroup group);
  void cleanupFinishedSounds();

  ma_engine engine_{};
  bool initialized_ = false;
  float masterVolume_ = 1.0f;
  SoundId nextSoundId_ = 1;
  std::array<GroupState, kGroupCount> groupStates_{};
  std::map<std::string, std::string> clips_;
  std::map<SoundId, ActiveSound> activeSounds_;
};

} // namespace DL

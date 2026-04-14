#pragma once

#include "miniaudio.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace DL {

enum class AudioGroup : std::uint8_t {
  Music = 0,
  SFX,
  Count
};

class AudioSystem {
public:
  using SoundId = std::uint64_t;
  static constexpr SoundId kInvalidSoundId = 0;
  static constexpr std::size_t kGroupCount =
      static_cast<std::size_t>(AudioGroup::Count);
  static constexpr std::size_t kSpectrumBandCount = 24;

  AudioSystem() = default;
  ~AudioSystem();

  AudioSystem(const AudioSystem &) = delete;
  AudioSystem &operator=(const AudioSystem &) = delete;

  bool init();
  void shutdown();
  [[nodiscard]] bool isInitialized() const { return initialized_; }

  bool loadClip(const std::string &name, const std::string &fileName,
                std::size_t oneShotPoolSize = 8);
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
  void setGroupVoiceLimit(AudioGroup group, std::size_t maxVoices);
  [[nodiscard]] std::size_t groupVoiceLimit(AudioGroup group) const;

  [[nodiscard]] static const char *groupLabel(AudioGroup group);
  [[nodiscard]] float masterVolume() const { return masterVolume_; }
  [[nodiscard]] const std::array<float, kSpectrumBandCount> &spectrumBands() const {
    return spectrumBands_;
  }
  [[nodiscard]] std::size_t loadedClipCount() const { return clips_.size(); }
  [[nodiscard]] std::size_t activeSoundCount() const {
    return activeSounds_.size();
  }
  [[nodiscard]] std::size_t activeSoundCount(AudioGroup group) const;

private:
  static constexpr std::size_t kInvalidClipVoiceIndex =
      static_cast<std::size_t>(-1);

  struct GroupState {
    ma_sound_group soundGroup{};
    bool initialized = false;
    float volume = 1.0f;
    bool muted = false;
  };

  struct ClipVoice {
    std::unique_ptr<ma_sound> sound;
    SoundId activeSoundId = kInvalidSoundId;
  };

  struct ClipData {
    std::string fileName;
    std::vector<ClipVoice> sfxPool;
    std::size_t nextSfxPoolIndex = 0;
    std::vector<float> analysisBands;
    std::size_t analysisFrameCount = 0;
    std::size_t analysisHopFrames = 1;
    ma_uint32 analysisSampleRate = 0;
  };

  struct ActiveSound {
    std::unique_ptr<ma_sound> ownedSound;
    ma_sound *sound = nullptr;
    bool looping = false;
    AudioGroup group = AudioGroup::SFX;
    std::string clipName;
    std::size_t clipVoiceIndex = kInvalidClipVoiceIndex;
    float volume = 1.0f;
    std::uint64_t startOrdinal = 0;
  };

  SoundId playPooledSfxOneShot(const std::string &clipName,
                               ClipData &clip,
                               float volume);
  SoundId playInternal(const std::string &name, AudioGroup group, bool loop,
                       float volume);
  bool initGroups();
  void uninitGroups();
  void applyGroupGain(AudioGroup group);
  bool ensureSfxPool(ClipData &clip, std::size_t desiredSize);
  bool buildClipAnalysis(ClipData &clip);
  void updateSpectrum();
  bool enforceGroupVoiceLimit(AudioGroup group);
  void releaseActiveSoundResources(ActiveSound &activeSound);
  void removeActiveSound(SoundId id);
  [[nodiscard]] static std::size_t toGroupIndex(AudioGroup group);
  void cleanupFinishedSounds();

  ma_engine engine_{};
  bool initialized_ = false;
  float masterVolume_ = 1.0f;
  SoundId nextSoundId_ = 1;
  std::uint64_t soundOrdinalCounter_ = 1;
  std::array<GroupState, kGroupCount> groupStates_{};
  std::array<std::size_t, kGroupCount> groupVoiceLimits_ = {1, 32};
  std::array<float, kSpectrumBandCount> spectrumBands_{};
  std::map<std::string, ClipData> clips_;
  std::map<SoundId, ActiveSound> activeSounds_;
};

} // namespace DL

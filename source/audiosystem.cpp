#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audiosystem.h"
#include "logger.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace DL {
namespace {
constexpr float kPi = 3.14159265358979323846f;

float bandCenterFrequency(std::size_t bandIndex, std::size_t bandCount,
                          float minFrequency, float maxFrequency) {
  if (bandCount <= 1) {
    return minFrequency;
  }
  const float t =
      static_cast<float>(bandIndex) / static_cast<float>(bandCount - 1);
  const float ratio = maxFrequency / minFrequency;
  return minFrequency * std::pow(ratio, t);
}

float goertzelMagnitude(const float *samples, std::size_t sampleCount,
                        float sampleRate, float targetFrequency) {
  if (samples == nullptr || sampleCount == 0 || sampleRate <= 0.0f) {
    return 0.0f;
  }
  const float omega = (2.0f * kPi * targetFrequency) / sampleRate;
  const float coeff = 2.0f * std::cos(omega);
  float sPrev = 0.0f;
  float sPrev2 = 0.0f;
  for (std::size_t n = 0; n < sampleCount; ++n) {
    const float s = samples[n] + coeff * sPrev - sPrev2;
    sPrev2 = sPrev;
    sPrev = s;
  }
  const float power = sPrev2 * sPrev2 + sPrev * sPrev - coeff * sPrev * sPrev2;
  return std::sqrt(std::max(0.0f, power)) / static_cast<float>(sampleCount);
}
} // namespace

AudioSystem::~AudioSystem() { shutdown(); }

bool AudioSystem::init() {
  if (initialized_) {
    return true;
  }

  if (ma_engine_init(nullptr, &engine_) != MA_SUCCESS) {
    LogError("Failed to initialize audio engine");
    return false;
  }

  if (!initGroups()) {
    ma_engine_uninit(&engine_);
    LogError("Failed to initialize audio groups");
    return false;
  }

  initialized_ = true;
  setMasterVolume(masterVolume_);
  LogInfo("Audio engine initialized");
  return true;
}

void AudioSystem::shutdown() {
  if (!initialized_) {
    return;
  }

  stopAll();
  for (auto &[name, clip] : clips_) {
    for (auto &voice : clip.sfxPool) {
      if (voice.sound) {
        ma_sound_uninit(voice.sound.get());
      }
      voice.activeSoundId = kInvalidSoundId;
    }
    if (clip.analysisDecoderInitialized && clip.analysisDecoder) {
      ma_decoder_uninit(clip.analysisDecoder.get());
      clip.analysisDecoderInitialized = false;
    }
  }
  clips_.clear();
  spectrumBands_.fill(0.0f);
  spectrumNormalizationPeak_ = 0.65f;

  uninitGroups();
  ma_engine_uninit(&engine_);
  initialized_ = false;
  nextSoundId_ = 1;
  soundOrdinalCounter_ = 1;
  LogInfo("Audio engine shut down");
}

bool AudioSystem::loadClip(const std::string &name, const std::string &fileName,
                           std::size_t oneShotPoolSize) {
  if (name.empty() || fileName.empty()) {
    LogWarn("Audio clip load ignored", "empty clip name or path");
    return false;
  }

  if (!initialized_ && !init()) {
    return false;
  }

  auto existing = clips_.find(name);
  if (existing != clips_.end()) {
    if (existing->second.fileName != fileName) {
      LogWarn("Audio clip already loaded with different file", name);
    }
    if (oneShotPoolSize > existing->second.sfxPool.size() &&
        !ensureSfxPool(existing->second, oneShotPoolSize)) {
      return false;
    }
    if (!existing->second.analysisDecoderInitialized) {
      initLiveDecoder(existing->second);
    }
    return true;
  }

  ClipData clip;
  clip.fileName = fileName;
  auto [it, inserted] = clips_.emplace(name, std::move(clip));
  if (!inserted) {
    return false;
  }
  if (!ensureSfxPool(it->second, oneShotPoolSize)) {
    clips_.erase(it);
    return false;
  }
  initLiveDecoder(it->second);
  return true;
}

bool AudioSystem::hasClip(const std::string &name) const {
  return clips_.find(name) != clips_.end();
}

AudioSystem::SoundId AudioSystem::playOneShot(const std::string &name,
                                              AudioGroup group, float volume) {
  if (!initialized_ && !init()) {
    return kInvalidSoundId;
  }

  const auto clipIt = clips_.find(name);
  if (clipIt == clips_.end()) {
    LogWarn("Audio clip not loaded", name);
    return kInvalidSoundId;
  }

  cleanupFinishedSounds();
  if (group == AudioGroup::SFX) {
    return playPooledSfxOneShot(name, clipIt->second, volume);
  }
  return playInternal(name, group, false, volume);
}

AudioSystem::SoundId AudioSystem::playLoop(const std::string &name,
                                           AudioGroup group, float volume) {
  return playInternal(name, group, true, volume);
}

void AudioSystem::stop(SoundId id) { removeActiveSound(id); }

void AudioSystem::stopAll() {
  while (!activeSounds_.empty()) {
    removeActiveSound(activeSounds_.begin()->first);
  }
}

void AudioSystem::update() {
  cleanupFinishedSounds();
  updateSpectrum();
}

void AudioSystem::setMasterVolume(float volume) {
  masterVolume_ = std::max(0.0f, volume);
  if (initialized_) {
    ma_engine_set_volume(&engine_, masterVolume_);
  }
}

void AudioSystem::setGroupVolume(AudioGroup group, float volume) {
  const std::size_t index = toGroupIndex(group);
  if (index >= kGroupCount) {
    return;
  }

  groupStates_[index].volume = std::max(0.0f, volume);
  applyGroupGain(group);
}

float AudioSystem::groupVolume(AudioGroup group) const {
  const std::size_t index = toGroupIndex(group);
  if (index >= kGroupCount) {
    return 1.0f;
  }
  return groupStates_[index].volume;
}

void AudioSystem::setGroupMuted(AudioGroup group, bool muted) {
  const std::size_t index = toGroupIndex(group);
  if (index >= kGroupCount) {
    return;
  }

  groupStates_[index].muted = muted;
  applyGroupGain(group);
}

bool AudioSystem::isGroupMuted(AudioGroup group) const {
  const std::size_t index = toGroupIndex(group);
  if (index >= kGroupCount) {
    return false;
  }
  return groupStates_[index].muted;
}

void AudioSystem::setGroupVoiceLimit(AudioGroup group, std::size_t maxVoices) {
  const std::size_t index = toGroupIndex(group);
  if (index >= kGroupCount) {
    return;
  }
  groupVoiceLimits_[index] = maxVoices;
}

std::size_t AudioSystem::groupVoiceLimit(AudioGroup group) const {
  const std::size_t index = toGroupIndex(group);
  if (index >= kGroupCount) {
    return 0;
  }
  return groupVoiceLimits_[index];
}

const char *AudioSystem::groupLabel(AudioGroup group) {
  switch (group) {
  case AudioGroup::Music:
    return "Music";
  case AudioGroup::SFX:
    return "SFX";
  case AudioGroup::Count:
    break;
  }
  return "Unknown";
}

std::size_t AudioSystem::activeSoundCount(AudioGroup group) const {
  std::size_t count = 0;
  for (const auto &[id, active] : activeSounds_) {
    if (active.group == group) {
      ++count;
    }
  }
  return count;
}

AudioSystem::SoundId
AudioSystem::playPooledSfxOneShot(const std::string &clipName, ClipData &clip,
                                  float volume) {
  if (clip.sfxPool.empty()) {
    return playInternal(clipName, AudioGroup::SFX, false, volume);
  }

  if (!enforceGroupVoiceLimit(AudioGroup::SFX)) {
    LogWarn("SFX voice limit reached", clipName);
    return kInvalidSoundId;
  }

  const std::size_t poolSize = clip.sfxPool.size();
  if (poolSize == 0) {
    return kInvalidSoundId;
  }
  std::size_t selected = kInvalidClipVoiceIndex;
  for (std::size_t offset = 0; offset < poolSize; ++offset) {
    const std::size_t index = (clip.nextSfxPoolIndex + offset) % poolSize;
    const SoundId activeId = clip.sfxPool[index].activeSoundId;
    if (activeId == kInvalidSoundId) {
      selected = index;
      break;
    }
    if (activeSounds_.find(activeId) == activeSounds_.end()) {
      clip.sfxPool[index].activeSoundId = kInvalidSoundId;
      selected = index;
      break;
    }
  }

  if (selected == kInvalidClipVoiceIndex) {
    selected = clip.nextSfxPoolIndex % poolSize;
    const SoundId stealId = clip.sfxPool[selected].activeSoundId;
    if (stealId != kInvalidSoundId) {
      removeActiveSound(stealId);
    }
  }
  clip.nextSfxPoolIndex = (selected + 1) % poolSize;

  auto &voice = clip.sfxPool[selected];
  if (!voice.sound) {
    return kInvalidSoundId;
  }

  ma_sound_stop(voice.sound.get());
  ma_sound_seek_to_pcm_frame(voice.sound.get(), 0);
  ma_sound_set_looping(voice.sound.get(), MA_FALSE);
  ma_sound_set_volume(voice.sound.get(), std::max(0.0f, volume));
  if (ma_sound_start(voice.sound.get()) != MA_SUCCESS) {
    LogError("Unable to start pooled sound", clip.fileName);
    return kInvalidSoundId;
  }

  const SoundId soundId = nextSoundId_++;
  voice.activeSoundId = soundId;

  ActiveSound active;
  active.sound = voice.sound.get();
  active.looping = false;
  active.group = AudioGroup::SFX;
  active.clipName = clipName;
  active.clipVoiceIndex = selected;
  active.volume = std::max(0.0f, volume);
  active.startOrdinal = soundOrdinalCounter_++;
  activeSounds_[soundId] = std::move(active);
  return soundId;
}

AudioSystem::SoundId AudioSystem::playInternal(const std::string &name,
                                               AudioGroup group, bool loop,
                                               float volume) {
  if (!initialized_ && !init()) {
    return kInvalidSoundId;
  }

  const std::size_t groupIndex = toGroupIndex(group);
  if (groupIndex >= kGroupCount || !groupStates_[groupIndex].initialized) {
    LogError("Audio group unavailable", groupLabel(group));
    return kInvalidSoundId;
  }

  const auto clipIt = clips_.find(name);
  if (clipIt == clips_.end()) {
    LogWarn("Audio clip not loaded", name);
    return kInvalidSoundId;
  }

  cleanupFinishedSounds();
  if (!enforceGroupVoiceLimit(group)) {
    LogWarn("Audio group voice limit reached", groupLabel(group));
    return kInvalidSoundId;
  }

  auto sound = std::make_unique<ma_sound>();
  if (ma_sound_init_from_file(&engine_, clipIt->second.fileName.c_str(), 0,
                              &groupStates_[groupIndex].soundGroup, nullptr,
                              sound.get()) != MA_SUCCESS) {
    LogError("Unable to create sound", clipIt->second.fileName);
    return kInvalidSoundId;
  }

  ma_sound_set_looping(sound.get(), loop ? MA_TRUE : MA_FALSE);
  ma_sound_set_volume(sound.get(), std::max(0.0f, volume));
  if (ma_sound_start(sound.get()) != MA_SUCCESS) {
    LogError("Unable to start sound", clipIt->second.fileName);
    ma_sound_uninit(sound.get());
    return kInvalidSoundId;
  }

  const SoundId soundId = nextSoundId_++;
  ActiveSound active;
  active.ownedSound = std::move(sound);
  active.sound = active.ownedSound.get();
  active.looping = loop;
  active.group = group;
  active.clipName = name;
  active.volume = std::max(0.0f, volume);
  active.startOrdinal = soundOrdinalCounter_++;
  activeSounds_[soundId] = std::move(active);
  return soundId;
}

bool AudioSystem::initGroups() {
  for (std::size_t i = 0; i < kGroupCount; ++i) {
    auto &state = groupStates_[i];
    if (ma_sound_group_init(&engine_, 0, nullptr, &state.soundGroup) !=
        MA_SUCCESS) {
      for (std::size_t j = 0; j < i; ++j) {
        if (groupStates_[j].initialized) {
          ma_sound_group_uninit(&groupStates_[j].soundGroup);
          groupStates_[j].initialized = false;
        }
      }
      return false;
    }
    state.initialized = true;
    applyGroupGain(static_cast<AudioGroup>(i));
  }
  return true;
}

void AudioSystem::uninitGroups() {
  for (std::size_t i = 0; i < kGroupCount; ++i) {
    auto &state = groupStates_[i];
    if (!state.initialized) {
      continue;
    }
    ma_sound_group_uninit(&state.soundGroup);
    state.initialized = false;
  }
}

void AudioSystem::applyGroupGain(AudioGroup group) {
  const std::size_t index = toGroupIndex(group);
  if (index >= kGroupCount) {
    return;
  }

  auto &state = groupStates_[index];
  if (!state.initialized) {
    return;
  }

  const float gain = state.muted ? 0.0f : state.volume;
  ma_sound_group_set_volume(&state.soundGroup, gain);
}

bool AudioSystem::ensureSfxPool(ClipData &clip, std::size_t desiredSize) {
  const std::size_t sfxIndex = toGroupIndex(AudioGroup::SFX);
  if (sfxIndex >= kGroupCount || !groupStates_[sfxIndex].initialized) {
    return false;
  }
  if (desiredSize == 0 || clip.sfxPool.size() >= desiredSize) {
    return true;
  }

  const std::size_t oldSize = clip.sfxPool.size();
  clip.sfxPool.reserve(desiredSize);
  for (std::size_t i = oldSize; i < desiredSize; ++i) {
    ClipVoice voice;
    voice.sound = std::make_unique<ma_sound>();
    if (ma_sound_init_from_file(&engine_, clip.fileName.c_str(), 0,
                                &groupStates_[sfxIndex].soundGroup, nullptr,
                                voice.sound.get()) != MA_SUCCESS) {
      LogError("Unable to initialize pooled SFX voice", clip.fileName);
      if (voice.sound) {
        ma_sound_uninit(voice.sound.get());
      }
      for (std::size_t rollback = oldSize; rollback < clip.sfxPool.size();
           ++rollback) {
        if (clip.sfxPool[rollback].sound) {
          ma_sound_uninit(clip.sfxPool[rollback].sound.get());
        }
      }
      clip.sfxPool.resize(oldSize);
      return false;
    }
    ma_sound_set_looping(voice.sound.get(), MA_FALSE);
    ma_sound_stop(voice.sound.get());
    clip.sfxPool.push_back(std::move(voice));
  }
  if (clip.nextSfxPoolIndex >= clip.sfxPool.size()) {
    clip.nextSfxPoolIndex = 0;
  }
  return true;
}

bool AudioSystem::initLiveDecoder(ClipData &clip) { // static
  if (clip.analysisDecoderInitialized) {
    return true;
  }
  clip.analysisDecoder = std::make_unique<ma_decoder>();
  const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, 0);
  if (ma_decoder_init_file(clip.fileName.c_str(), &config,
                           clip.analysisDecoder.get()) != MA_SUCCESS) {
    LogWarn("Unable to open clip for live spectrum analysis", clip.fileName);
    clip.analysisDecoder.reset();
    return false;
  }
  clip.analysisSampleRate = clip.analysisDecoder->outputSampleRate;
  clip.analysisDecoderInitialized = true;
  return true;
}

void AudioSystem::updateSpectrum() {
  constexpr std::size_t kWindowSize = 256;
  constexpr float kNoiseFloor = 0.02f;
  constexpr float kMinPeakForNormalize = 0.08f;
  constexpr float kBaseNormalizationPeak = 0.65f;
  constexpr float kPeakAttack = 0.10f;
  constexpr float kPeakRelease = 0.03f;
  std::array<float, kSpectrumBandCount> targetBands{};
  targetBands.fill(0.0f);
  float framePeak = 0.0f;

  std::vector<float> windowSamples(kWindowSize, 0.0f);

  for (const auto &[id, active] : activeSounds_) {
    if (active.sound == nullptr || active.clipName.empty()) {
      continue;
    }
    if (active.group != AudioGroup::Music) {
      continue;
    }

    const auto clipIt = clips_.find(active.clipName);
    if (clipIt == clips_.end()) {
      continue;
    }
    ClipData &clip = clipIt->second;
    if (!clip.analysisDecoderInitialized || !clip.analysisDecoder ||
        clip.analysisSampleRate == 0) {
      continue;
    }

    float cursorSeconds = 0.0f;
    if (ma_sound_get_cursor_in_seconds(active.sound, &cursorSeconds) !=
        MA_SUCCESS) {
      continue;
    }

    const std::size_t groupIndex = toGroupIndex(active.group);
    if (groupIndex >= kGroupCount || groupStates_[groupIndex].muted) {
      continue;
    }
    const float gain =
        active.volume * masterVolume_ * groupStates_[groupIndex].volume;
    if (gain <= 0.0f) {
      continue;
    }

    // Use playback seconds to avoid frame-domain mismatch between mixer and
    // decoder.
    const ma_uint64 targetFrame =
        static_cast<ma_uint64>(std::max(0.0f, cursorSeconds) *
                               static_cast<float>(clip.analysisSampleRate));
    if (ma_decoder_seek_to_pcm_frame(clip.analysisDecoder.get(), targetFrame) !=
        MA_SUCCESS) {
      continue;
    }
    ma_uint64 framesRead = 0;
    if (ma_decoder_read_pcm_frames(clip.analysisDecoder.get(),
                                   windowSamples.data(), kWindowSize,
                                   &framesRead) != MA_SUCCESS) {
      continue;
    }
    // Zero-pad if we read fewer frames than the window (e.g. near end of file)
    const ma_uint64 readEnd =
        std::min(framesRead, static_cast<ma_uint64>(kWindowSize));
    for (auto n = static_cast<std::size_t>(readEnd); n < kWindowSize; ++n) {
      windowSamples[n] = 0.0f;
    }

    // Apply Hanning window
    for (std::size_t n = 0; n < kWindowSize; ++n) {
      const float w =
          0.5f - 0.5f * std::cos((2.0f * kPi * static_cast<float>(n)) /
                                 static_cast<float>(kWindowSize - 1));
      windowSamples[n] *= w;
    }

    const float nyquist = static_cast<float>(clip.analysisSampleRate) * 0.5f;
    const float minFrequency = 60.0f;
    const float maxFrequency = std::max(minFrequency + 1.0f, nyquist - 40.0f);

    for (std::size_t band = 0; band < kSpectrumBandCount; ++band) {
      const float frequency = bandCenterFrequency(band, kSpectrumBandCount,
                                                  minFrequency, maxFrequency);
      const float magnitude = goertzelMagnitude(
          windowSamples.data(), kWindowSize,
          static_cast<float>(clip.analysisSampleRate), frequency);
      const float compressed = std::log1p(magnitude * 16.0f);
      if (!std::isfinite(compressed))
        continue;
      targetBands[band] += compressed * gain;
      framePeak = std::max(framePeak, targetBands[band]);
    }
  }

  // Don't normalize silence/noise; that amplifies low-band residue and makes
  // bars stick.
  const bool hasUsableSignal = framePeak >= kMinPeakForNormalize;
  const float desiredNormalizationPeak =
      hasUsableSignal ? std::max(kBaseNormalizationPeak, framePeak)
                      : kBaseNormalizationPeak;
  const float peakBlend = desiredNormalizationPeak > spectrumNormalizationPeak_
                              ? kPeakAttack
                              : kPeakRelease;
  spectrumNormalizationPeak_ +=
      (desiredNormalizationPeak - spectrumNormalizationPeak_) * peakBlend;
  const float normalizationPeak =
      std::max(kBaseNormalizationPeak, spectrumNormalizationPeak_);

  for (std::size_t band = 0; band < kSpectrumBandCount; ++band) {
    const float energy = std::max(0.0f, targetBands[band] - kNoiseFloor);
    const float target =
        hasUsableSignal
            ? std::clamp(
                  std::pow(std::clamp(energy / normalizationPeak, 0.0f, 1.0f),
                           0.7f),
                  0.0f, 2.0f)
            : 0.0f;
    const float current = spectrumBands_[band];
    const float blend = target > current ? 0.45f : 0.22f;
    spectrumBands_[band] =
        std::clamp(current + (target - current) * blend, 0.0f, 2.0f);
  }
}

bool AudioSystem::enforceGroupVoiceLimit(AudioGroup group) {
  const std::size_t groupIndex = toGroupIndex(group);
  if (groupIndex >= kGroupCount) {
    return false;
  }

  const std::size_t limit = groupVoiceLimits_[groupIndex];
  if (limit == 0) {
    return false;
  }

  while (activeSoundCount(group) >= limit) {
    SoundId candidate = kInvalidSoundId;
    std::uint64_t oldest = std::numeric_limits<std::uint64_t>::max();
    for (const auto &[id, active] : activeSounds_) {
      if (active.group != group || active.looping) {
        continue;
      }
      if (active.startOrdinal < oldest) {
        oldest = active.startOrdinal;
        candidate = id;
      }
    }
    if (candidate == kInvalidSoundId) {
      oldest = std::numeric_limits<std::uint64_t>::max();
      for (const auto &[id, active] : activeSounds_) {
        if (active.group != group) {
          continue;
        }
        if (active.startOrdinal < oldest) {
          oldest = active.startOrdinal;
          candidate = id;
        }
      }
    }
    if (candidate == kInvalidSoundId) {
      return false;
    }
    removeActiveSound(candidate);
  }

  return true;
}

void AudioSystem::releaseActiveSoundResources(ActiveSound &activeSound) {
  if (activeSound.sound == nullptr) {
    return;
  }

  ma_sound_stop(activeSound.sound);
  if (activeSound.clipVoiceIndex != kInvalidClipVoiceIndex) {
    auto clipIt = clips_.find(activeSound.clipName);
    if (clipIt != clips_.end() &&
        activeSound.clipVoiceIndex < clipIt->second.sfxPool.size()) {
      clipIt->second.sfxPool[activeSound.clipVoiceIndex].activeSoundId =
          kInvalidSoundId;
    }
  } else if (activeSound.ownedSound) {
    ma_sound_uninit(activeSound.ownedSound.get());
    activeSound.ownedSound.reset();
  }

  activeSound.sound = nullptr;
}

void AudioSystem::removeActiveSound(SoundId id) {
  auto it = activeSounds_.find(id);
  if (it == activeSounds_.end()) {
    return;
  }

  releaseActiveSoundResources(it->second);
  activeSounds_.erase(it);
}

std::size_t AudioSystem::toGroupIndex(AudioGroup group) {
  return static_cast<std::size_t>(group);
}

void AudioSystem::cleanupFinishedSounds() {
  for (auto it = activeSounds_.begin(); it != activeSounds_.end();) {
    if (!it->second.looping && it->second.sound != nullptr &&
        ma_sound_is_playing(it->second.sound) == MA_FALSE) {
      releaseActiveSoundResources(it->second);
      it = activeSounds_.erase(it);
      continue;
    }
    ++it;
  }
}

} // namespace DL

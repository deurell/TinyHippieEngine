#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audiosystem.h"
#include "logger.h"
#include <algorithm>

namespace DL {

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
  uninitGroups();
  ma_engine_uninit(&engine_);
  initialized_ = false;
  clips_.clear();
  nextSoundId_ = 1;
  LogInfo("Audio engine shut down");
}

bool AudioSystem::loadClip(const std::string &name, const std::string &fileName) {
  if (name.empty() || fileName.empty()) {
    LogWarn("Audio clip load ignored", "empty clip name or path");
    return false;
  }

  if (!initialized_ && !init()) {
    return false;
  }

  auto it = clips_.find(name);
  if (it != clips_.end()) {
    return true;
  }

  clips_[name] = fileName;
  return true;
}

bool AudioSystem::hasClip(const std::string &name) const {
  return clips_.find(name) != clips_.end();
}

AudioSystem::SoundId AudioSystem::playOneShot(const std::string &name,
                                              AudioGroup group,
                                              float volume) {
  return playInternal(name, group, false, volume);
}

AudioSystem::SoundId AudioSystem::playLoop(const std::string &name,
                                           AudioGroup group,
                                           float volume) {
  return playInternal(name, group, true, volume);
}

void AudioSystem::stop(SoundId id) {
  auto it = activeSounds_.find(id);
  if (it == activeSounds_.end()) {
    return;
  }

  ma_sound_stop(it->second.sound.get());
  ma_sound_uninit(it->second.sound.get());
  activeSounds_.erase(it);
}

void AudioSystem::stopAll() {
  for (auto &entry : activeSounds_) {
    ma_sound_stop(entry.second.sound.get());
    ma_sound_uninit(entry.second.sound.get());
  }
  activeSounds_.clear();
}

void AudioSystem::update() { cleanupFinishedSounds(); }

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
  for (const auto &entry : activeSounds_) {
    if (entry.second.group == group) {
      ++count;
    }
  }
  return count;
}

AudioSystem::SoundId AudioSystem::playInternal(const std::string &name,
                                               AudioGroup group,
                                               bool loop,
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

  auto sound = std::make_unique<ma_sound>();
  if (ma_sound_init_from_file(&engine_, clipIt->second.c_str(), 0,
                              &groupStates_[groupIndex].soundGroup, nullptr,
                              sound.get()) != MA_SUCCESS) {
    LogError("Unable to create sound", clipIt->second);
    return kInvalidSoundId;
  }

  ma_sound_set_looping(sound.get(), loop ? MA_TRUE : MA_FALSE);
  ma_sound_set_volume(sound.get(), std::max(0.0f, volume));

  if (ma_sound_start(sound.get()) != MA_SUCCESS) {
    LogError("Unable to start sound", clipIt->second);
    ma_sound_uninit(sound.get());
    return kInvalidSoundId;
  }

  const SoundId soundId = nextSoundId_++;
  activeSounds_[soundId] = {std::move(sound), loop, group};
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

std::size_t AudioSystem::toGroupIndex(AudioGroup group) {
  return static_cast<std::size_t>(group);
}

void AudioSystem::cleanupFinishedSounds() {
  for (auto it = activeSounds_.begin(); it != activeSounds_.end();) {
    if (!it->second.looping &&
        ma_sound_is_playing(it->second.sound.get()) == MA_FALSE) {
      ma_sound_uninit(it->second.sound.get());
      it = activeSounds_.erase(it);
      continue;
    }
    ++it;
  }
}

} // namespace DL

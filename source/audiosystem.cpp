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
  ma_engine_uninit(&engine_);
  initialized_ = false;
  clips_.clear();
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
                                              float volume) {
  return playInternal(name, false, volume);
}

AudioSystem::SoundId AudioSystem::playLoop(const std::string &name,
                                           float volume) {
  return playInternal(name, true, volume);
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

AudioSystem::SoundId AudioSystem::playInternal(const std::string &name,
                                               bool loop, float volume) {
  if (!initialized_ && !init()) {
    return kInvalidSoundId;
  }

  const auto clipIt = clips_.find(name);
  if (clipIt == clips_.end()) {
    LogWarn("Audio clip not loaded", name);
    return kInvalidSoundId;
  }

  cleanupFinishedSounds();

  auto sound = std::make_unique<ma_sound>();
  if (ma_sound_init_from_file(&engine_, clipIt->second.c_str(), 0, nullptr,
                              nullptr, sound.get()) != MA_SUCCESS) {
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
  activeSounds_[soundId] = {std::move(sound), loop};
  return soundId;
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

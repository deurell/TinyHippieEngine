//
// Created by Mikael Deurell on 2021-09-16.
//

#include "audioplayer.h"
#include <iostream>
#include <locale>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) {
        return;
    }

    ma_uint64 framesRead;
    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);
}

AudioPlayer::~AudioPlayer() {
  for (auto &audioData : audioData_) {
    ma_device_uninit(audioData.second.device.get());
    ma_decoder_uninit(audioData.second.decoder.get());
  }
}

void AudioPlayer::load(const std::string &name, const std::string &fileName) {
  if (audioData_.find(name) != audioData_.end()) {
    std::cout << "Audio with name " << name << " already loaded." << std::endl;
    return;
  }
  AudioData audioData;
  audioData.decoder = std::make_unique<ma_decoder>();
  audioData.device = std::make_unique<ma_device>();
  ma_result result;
  
  if (ma_decoder_init_file(fileName.c_str(), nullptr, audioData.decoder.get()) !=
      MA_SUCCESS) {
    std::cout << "Unable to load audio file " << fileName << std::endl;
    return;
  }

  ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = audioData.decoder->outputFormat;
  deviceConfig.playback.channels = audioData.decoder->outputChannels;
  deviceConfig.sampleRate = audioData.decoder->outputSampleRate;
  deviceConfig.dataCallback = data_callback;
  deviceConfig.pUserData = audioData.decoder.get();

  if (ma_device_init(nullptr, &deviceConfig, audioData.device.get()) != MA_SUCCESS) {
    std::cout << "Unable to initialize playback device." << std::endl;
    ma_decoder_uninit(audioData.decoder.get());
    return;
  }

  audioData_[name] = std::move(audioData);
}

void AudioPlayer::play(const std::string &name) {
  if (audioData_.find(name) == audioData_.end()) {
    std::cout << "Audio with name " << name << " not loaded." << std::endl;
    return;
  }

  auto &audioData = audioData_[name];
  if (ma_device_start(audioData.device.get()) != MA_SUCCESS) {
    std::cout << "Unable to start playback device." << std::endl;
    ma_device_uninit(audioData.device.get());
    ma_decoder_uninit(audioData.decoder.get());
    return;
  }
}

void AudioPlayer::stop(const std::string &name) {
  if (audioData_.find(name) == audioData_.end()) {
    std::cout << "Audio with name " << name << " not loaded." << std::endl;
    return;
  }

  auto &audioData = audioData_[name];
  ma_device_stop(audioData.device.get());
}

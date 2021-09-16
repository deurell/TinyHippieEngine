//
// Created by Mikael Deurell on 2021-09-16.
//

#include "audioplayer.h"
#include <iostream>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

void data_callback(ma_device *pDevice, void *pOutput,
                   [[maybe_unused]] const void *pInput,
                   ma_uint32 frameCount) {
  auto *pDecoder = (ma_decoder *)pDevice->pUserData;
  if (pDecoder == nullptr) {
    return;
  }
  ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount);
}

AudioPlayer::~AudioPlayer() {
  ma_device_uninit(&mDevice);
  ma_decoder_uninit(&mDecoder);
}

void AudioPlayer::load(const std::string &fileName) {
  ma_result result;
  ma_device_config deviceConfig;

  result = ma_decoder_init_file("./Resources/unlock.wav", nullptr, &mDecoder);
  if (result != MA_SUCCESS) {
    std::cout << "Unable to load sound file." << std::endl;
  }

  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = mDecoder.outputFormat;
  deviceConfig.playback.channels = mDecoder.outputChannels;
  deviceConfig.sampleRate = mDecoder.outputSampleRate;
  deviceConfig.dataCallback = data_callback;
  deviceConfig.pUserData = &mDecoder;

  if (ma_device_init(nullptr, &deviceConfig, &mDevice) != MA_SUCCESS) {
    std::cout << "Failed to open playback device." << std::endl;
    ma_decoder_uninit(&mDecoder);
  }
}

void AudioPlayer::play() {
  if (ma_device_start(&mDevice) != MA_SUCCESS) {
    std::cout << "Unable to play audio." << std::endl;
    ma_device_uninit(&mDevice);
    ma_decoder_uninit(&mDecoder);
  }
}

void AudioPlayer::stop() { ma_device_stop(&mDevice); }


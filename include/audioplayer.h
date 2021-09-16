//
// Created by Mikael Deurell on 2021-09-16.
//
#pragma once
#include <string>
#include "miniaudio.h"

class AudioPlayer {
public:
  AudioPlayer() = default;
  ~AudioPlayer();

  void load(const std::string &fileName);
  void play();
  void stop();

private:
  ma_device mDevice;
  ma_decoder mDecoder;
};

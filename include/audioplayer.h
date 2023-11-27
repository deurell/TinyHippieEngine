//
// Created by Mikael Deurell on 2021-09-16.
//
#pragma once
#include <string>
#include "miniaudio.h"
#include <map>

struct AudioData {
  std::unique_ptr<ma_decoder> decoder;
  std::unique_ptr<ma_device> device;
};

class AudioPlayer {
public:
  AudioPlayer() = default;
  ~AudioPlayer();

  void load(const std::string &name, const std::string &fileName);
  void play(const std::string &name);
  void stop(const std::string &name);

private:
  std::map<std::string, AudioData> audioData_;

};

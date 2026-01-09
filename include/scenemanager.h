#pragma once

#include "iscene.h"
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace DL {

class SceneManager {
public:
  using SceneFactory =
      std::function<std::unique_ptr<DL::IScene>(std::string_view glslVersion)>;

  void registerScene(SceneFactory factory) {
    factories_.push_back(std::move(factory));
  }

  std::unique_ptr<DL::IScene> createCurrent(std::string_view glsl) const {
    if (factories_.empty()) {
      return nullptr;
    }
    return factories_[currentIndex_](glsl);
  }

  bool hasScenes() const { return !factories_.empty(); }

  void next() {
    if (!factories_.empty()) {
      currentIndex_ = (currentIndex_ + 1) % factories_.size();
    }
  }

  void previous() {
    if (!factories_.empty()) {
      currentIndex_ = (currentIndex_ + factories_.size() - 1) % factories_.size();
    }
  }

private:
  std::vector<SceneFactory> factories_;
  std::size_t currentIndex_ = 0;
};

} // namespace DL

#pragma once

#include "spritenode.h"
#include <vector>

struct SpriteAnimationFrame {
  glm::vec4 sourceRectPixels{0.0f, 0.0f, -1.0f, -1.0f};
};

struct SpriteAnimationConfig {
  std::vector<SpriteAnimationFrame> frames;
  float fps = 8.0f;
  bool playing = true;
  bool looping = true;
};

class SpriteAnimationNode : public SpriteNode {
public:
  explicit SpriteAnimationNode(
      std::string imagePath, basist::etc1_global_selector_codebook *codeBook,
      DL::IRenderDevice *renderDevice,
      DL::RenderResourceCache *renderResourceCache = nullptr,
      DL::SceneNode *parentNode = nullptr, DL::Camera *camera = nullptr);

  void fixedUpdate(const DL::FrameContext &ctx) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "SpriteAnimationNode";
  }

  void setAnimation(SpriteAnimationConfig config);
  [[nodiscard]] const SpriteAnimationConfig &animation() const {
    return animation_;
  }
  [[nodiscard]] std::size_t currentFrameIndex() const {
    return currentFrameIndex_;
  }
  void setAnimationPlaying(bool playing) { animation_.playing = playing; }
  void setAnimationLooping(bool looping) { animation_.looping = looping; }
  void setAnimationFps(float fps) {
    if (fps > 0.0f) {
      animation_.fps = fps;
    }
  }

private:
  void applyCurrentFrame();

  SpriteAnimationConfig animation_;
  std::size_t currentFrameIndex_ = 0;
  float frameAccumulator_ = 0.0f;
};

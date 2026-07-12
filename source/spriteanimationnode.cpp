#include "spriteanimationnode.h"

#include <algorithm>
#include <utility>

SpriteAnimationNode::SpriteAnimationNode(
    std::string imagePath, basist::etc1_global_selector_codebook *codeBook,
    DL::IRenderDevice *renderDevice, DL::RenderResourceCache *renderResourceCache,
    DL::SceneNode *parentNode, DL::Camera *camera)
    : SpriteNode(std::move(imagePath), codeBook, renderDevice,
                 renderResourceCache, parentNode, camera) {}

void SpriteAnimationNode::fixedUpdate(const DL::FrameContext &ctx) {
  if (animation_.playing && animation_.frames.size() > 1u &&
      animation_.fps > 0.0f) {
    frameAccumulator_ += ctx.delta_time;
    const float frameSeconds = 1.0f / animation_.fps;
    while (frameAccumulator_ >= frameSeconds) {
      frameAccumulator_ -= frameSeconds;
      if (currentFrameIndex_ + 1u < animation_.frames.size()) {
        ++currentFrameIndex_;
      } else if (animation_.looping) {
        currentFrameIndex_ = 0u;
      } else {
        animation_.playing = false;
        currentFrameIndex_ = animation_.frames.size() - 1u;
        frameAccumulator_ = 0.0f;
        break;
      }
      applyCurrentFrame();
    }
  }

  SpriteNode::fixedUpdate(ctx);
}

void SpriteAnimationNode::setAnimation(SpriteAnimationConfig config) {
  animation_ = std::move(config);
  currentFrameIndex_ = 0u;
  frameAccumulator_ = 0.0f;
  applyCurrentFrame();
}

void SpriteAnimationNode::applyCurrentFrame() {
  if (animation_.frames.empty()) {
    return;
  }
  currentFrameIndex_ =
      std::min(currentFrameIndex_, animation_.frames.size() - 1u);
  setAtlasSourceRectPixels(
      animation_.frames[currentFrameIndex_].sourceRectPixels);
}

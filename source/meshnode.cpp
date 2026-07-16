#include "meshnode.h"

#include "meshrendercomponent.h"

MeshNode::MeshNode(std::string assetPath,
                   basist::etc1_global_selector_codebook *codeBook,
                   DL::IRenderDevice *renderDevice,
                   DL::MeshAssetCache *meshAssetCache,
                   DL::RenderResourceCache *renderResourceCache,
                   DL::SceneNode *parentNode, DL::Camera *camera)
    : SceneNode(parentNode), assetPath_(std::move(assetPath)),
      codeBook_(codeBook), camera_(camera), renderDevice_(renderDevice),
      meshAssetCache_(meshAssetCache), renderResourceCache_(renderResourceCache) {}

void MeshNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void MeshNode::update(const DL::FrameContext &ctx) {
  if (meshRenderComponent_ != nullptr) {
    meshRenderComponent_->updateAnimation(ctx.delta_time);
  }
  SceneNode::update(ctx);
}

void MeshNode::render(const DL::FrameContext &ctx) { SceneNode::render(ctx); }

void MeshNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void MeshNode::setDebugNormals(bool enabled) {
  debugNormals_ = enabled;
  if (meshRenderComponent_ != nullptr) {
    meshRenderComponent_->setDebugNormals(enabled);
  }
}

void MeshNode::setRenderSettings(
    const DL::MeshRenderSettings &settings) {
  renderSettings_ = settings;
  if (meshRenderComponent_ != nullptr) {
    meshRenderComponent_->setSettings(renderSettings_);
  }
}

void MeshNode::setAnimationPlaying(bool playing) {
  if (meshRenderComponent_ != nullptr) {
    meshRenderComponent_->setAnimationPlaying(playing);
  }
}

bool MeshNode::isAnimationPlaying() const {
  return meshRenderComponent_ != nullptr && meshRenderComponent_->isAnimationPlaying();
}

void MeshNode::setAnimationLooping(bool looping) {
  if (meshRenderComponent_ != nullptr) {
    meshRenderComponent_->setAnimationLooping(looping);
  }
}

bool MeshNode::isAnimationLooping() const {
  return meshRenderComponent_ != nullptr && meshRenderComponent_->isAnimationLooping();
}

void MeshNode::setAnimationPlaybackSpeed(float speed) {
  if (meshRenderComponent_ != nullptr) {
    meshRenderComponent_->setAnimationPlaybackSpeed(speed);
  }
}

float MeshNode::animationPlaybackSpeed() const {
  return meshRenderComponent_ != nullptr ? meshRenderComponent_->animationPlaybackSpeed()
                                    : 1.0f;
}

std::size_t MeshNode::findAnimationClipIndex(std::string_view name,
                                             std::size_t fallback) const {
  return meshRenderComponent_ != nullptr
             ? meshRenderComponent_->findAnimationClipIndex(name, fallback)
             : 0u;
}

void MeshNode::setAnimationClipIndex(std::size_t index) {
  if (meshRenderComponent_ != nullptr) {
    meshRenderComponent_->setAnimationClipIndex(index);
  }
}

std::size_t MeshNode::animationClipIndex() const {
  return meshRenderComponent_ != nullptr ? meshRenderComponent_->animationClipIndex() : 0u;
}

std::string_view MeshNode::animationClipName(std::size_t index) const {
  return meshRenderComponent_ != nullptr ? meshRenderComponent_->animationClipName(index)
                                    : std::string_view{};
}

std::size_t MeshNode::animationClipCount() const {
  return meshRenderComponent_ != nullptr ? meshRenderComponent_->animationClipCount() : 0u;
}

bool MeshNode::hasAnimations() const {
  return meshRenderComponent_ != nullptr && meshRenderComponent_->hasAnimations();
}

void MeshNode::applyAnimationBlend(const DL::AnimationBlendState &state) {
  if (meshRenderComponent_ != nullptr) {
    meshRenderComponent_->applyAnimationBlend(state);
  }
}

void MeshNode::setAnimationBlend(std::size_t baseClipIndex,
                                 std::size_t blendClipIndex, float weight) {
  if (meshRenderComponent_ != nullptr) {
    meshRenderComponent_->setAnimationBlend(baseClipIndex, blendClipIndex, weight);
  }
}

void MeshNode::setAnimationBlendByName(std::string_view baseClipName,
                                       std::string_view blendClipName,
                                       float weight) {
  if (meshRenderComponent_ != nullptr) {
    meshRenderComponent_->setAnimationBlendByName(baseClipName, blendClipName, weight);
  }
}

float MeshNode::animationBlendWeight() const {
  return meshRenderComponent_ != nullptr ? meshRenderComponent_->animationBlendWeight() : 0.0f;
}

void MeshNode::initCamera() {
  if (camera_ != nullptr) {
    return;
  }
  localCamera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 20.0f));
  localCamera_->lookAt({0.0f, 0.0f, 0.0f});
  camera_ = localCamera_.get();
}

void MeshNode::initComponents() {
  if (camera_ == nullptr || renderDevice_ == nullptr) {
    return;
  }

  auto renderer = std::make_unique<DL::MeshRenderComponent>(
      *camera_, *this,
      meshAssetCache_ != nullptr ? meshAssetCache_->load(assetPath_)
                                 : std::make_shared<DL::MeshAsset>(DL::loadMeshAsset(assetPath_)),
      codeBook_, renderDevice_, renderResourceCache_);
  meshRenderComponent_ = renderer.get();
  meshRenderComponent_->setDebugNormals(debugNormals_);
  meshRenderComponent_->setSettings(renderSettings_);
  addRenderComponent(std::move(renderer));
}

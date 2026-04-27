#include "meshnode.h"

#include "meshvisualizer.h"

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
  if (meshVisualizer_ != nullptr) {
    meshVisualizer_->updateAnimation(ctx.delta_time);
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
  if (meshVisualizer_ != nullptr) {
    meshVisualizer_->setDebugNormals(enabled);
  }
}

void MeshNode::setVisualizerSettings(
    const DL::MeshVisualizerSettings &settings) {
  visualizerSettings_ = settings;
  if (meshVisualizer_ != nullptr) {
    meshVisualizer_->setSettings(visualizerSettings_);
  }
}

void MeshNode::setAnimationPlaying(bool playing) {
  if (meshVisualizer_ != nullptr) {
    meshVisualizer_->setAnimationPlaying(playing);
  }
}

bool MeshNode::isAnimationPlaying() const {
  return meshVisualizer_ != nullptr && meshVisualizer_->isAnimationPlaying();
}

void MeshNode::setAnimationLooping(bool looping) {
  if (meshVisualizer_ != nullptr) {
    meshVisualizer_->setAnimationLooping(looping);
  }
}

bool MeshNode::isAnimationLooping() const {
  return meshVisualizer_ != nullptr && meshVisualizer_->isAnimationLooping();
}

void MeshNode::setAnimationPlaybackSpeed(float speed) {
  if (meshVisualizer_ != nullptr) {
    meshVisualizer_->setAnimationPlaybackSpeed(speed);
  }
}

float MeshNode::animationPlaybackSpeed() const {
  return meshVisualizer_ != nullptr ? meshVisualizer_->animationPlaybackSpeed()
                                    : 1.0f;
}

void MeshNode::setAnimationClipIndex(std::size_t index) {
  if (meshVisualizer_ != nullptr) {
    meshVisualizer_->setAnimationClipIndex(index);
  }
}

std::size_t MeshNode::animationClipIndex() const {
  return meshVisualizer_ != nullptr ? meshVisualizer_->animationClipIndex() : 0u;
}

std::string_view MeshNode::animationClipName(std::size_t index) const {
  return meshVisualizer_ != nullptr ? meshVisualizer_->animationClipName(index)
                                    : std::string_view{};
}

std::size_t MeshNode::animationClipCount() const {
  return meshVisualizer_ != nullptr ? meshVisualizer_->animationClipCount() : 0u;
}

bool MeshNode::hasAnimations() const {
  return meshVisualizer_ != nullptr && meshVisualizer_->hasAnimations();
}

void MeshNode::setAnimationBlend(std::size_t baseClipIndex,
                                 std::size_t blendClipIndex, float weight) {
  if (meshVisualizer_ != nullptr) {
    meshVisualizer_->setAnimationBlend(baseClipIndex, blendClipIndex, weight);
  }
}

float MeshNode::animationBlendWeight() const {
  return meshVisualizer_ != nullptr ? meshVisualizer_->animationBlendWeight() : 0.0f;
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

  auto visualizer = std::make_unique<DL::MeshVisualizer>(
      *camera_, *this,
      meshAssetCache_ != nullptr ? meshAssetCache_->load(assetPath_)
                                 : std::make_shared<DL::MeshAsset>(DL::loadMeshAsset(assetPath_)),
      codeBook_, renderDevice_, renderResourceCache_);
  meshVisualizer_ = visualizer.get();
  meshVisualizer_->setDebugNormals(debugNormals_);
  meshVisualizer_->setSettings(visualizerSettings_);
  addRenderComponent(std::move(visualizer));
}

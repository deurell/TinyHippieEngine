#include "meshnode.h"

#include "meshvisualizer.h"

MeshNode::MeshNode(std::string assetPath,
                   basist::etc1_global_selector_codebook *codeBook,
                   DL::IRenderDevice *renderDevice, DL::SceneNode *parentNode,
                   DL::Camera *camera)
    : SceneNode(parentNode), assetPath_(std::move(assetPath)),
      codeBook_(codeBook), camera_(camera), renderDevice_(renderDevice) {}

void MeshNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void MeshNode::update(const DL::FrameContext &ctx) { SceneNode::update(ctx); }

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
      "MeshVisualizer", *camera_, *this, DL::loadObjMeshAsset(assetPath_),
      codeBook_, renderDevice_);
  meshVisualizer_ = visualizer.get();
  meshVisualizer_->setDebugNormals(debugNormals_);
  meshVisualizer_->setSettings(visualizerSettings_);
  visualizers.emplace_back(std::move(visualizer));
}

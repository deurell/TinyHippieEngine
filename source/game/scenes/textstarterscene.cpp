#include "textstarterscene.h"

#include "logger.h"
#include <glm/gtc/quaternion.hpp>
#include <stdexcept>

TextStarterScene::TextStarterScene(
    DL::IRenderDevice *renderDevice,
    basist::etc1_global_selector_codebook *codeBook,
    DL::MeshAssetCache *meshAssetCache,
    DL::RenderResourceCache *renderResourceCache,
    std::filesystem::path scenePath)
    : SceneNode(nullptr), renderDevice_(renderDevice), codeBook_(codeBook),
      meshAssetCache_(meshAssetCache),
      renderResourceCache_(renderResourceCache),
      scenePath_(std::move(scenePath)) {
  setDebugName("text starter scene");
}

void TextStarterScene::init() {
  SceneNode::init();
  initCamera();
  loadTextScene();
  bindRuntimeNodes();
}

void TextStarterScene::fixedUpdate(const DL::FrameContext &ctx) {
  if (hero_ != nullptr) {
    heroYawRadians_ += ctx.delta_time * 0.45f;
    hero_->setLocalRotation(glm::quat(glm::vec3(0.0f, heroYawRadians_, 0.0f)));
  }
  SceneNode::fixedUpdate(ctx);
}

void TextStarterScene::update(const DL::FrameContext &ctx) {
  SceneNode::update(ctx);
}

void TextStarterScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void TextStarterScene::initCamera() {
  camera_ = std::make_unique<DL::Camera>(glm::vec3(3.2f, 2.4f, 4.2f));
  camera_->mFov = 36.0f;
  camera_->lookAt(glm::vec3(0.0f, 0.55f, 0.0f));
}

void TextStarterScene::loadTextScene() {
  const DL::SceneDescription description = DL::loadSceneDescription(scenePath_);
  for (const auto &nodeDescription : description.nodes) {
    addChild(DL::buildSceneNode(
        nodeDescription,
        {.renderDevice = renderDevice_,
         .codeBook = codeBook_,
         .meshAssetCache = meshAssetCache_,
         .renderResourceCache = renderResourceCache_,
         .camera = camera_.get()},
        this));
  }
  DL::Logger::instance().logEvent(DL::LogLevel::Info, "scene_description",
                                  "loaded " + description.name);
}

void TextStarterScene::bindRuntimeNodes() {
  hero_ = DL::findSceneNodeByName(*this, "hero");
  if (hero_ == nullptr) {
    throw std::runtime_error(
        "TextStarterScene requires a node named 'hero' for C++ behavior");
  }
}

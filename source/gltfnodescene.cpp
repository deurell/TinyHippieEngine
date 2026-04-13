#include "gltfnodescene.h"

#include "debugui.h"
#include "imgui.h"

GltfNodeScene::GltfNodeScene(DL::IRenderDevice *renderDevice,
                             basist::etc1_global_selector_codebook *codeBook)
    : SceneNode(nullptr), renderDevice_(renderDevice), codeBook_(codeBook) {}

void GltfNodeScene::init() {
  SceneNode::init();
  camera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 12.0f));
  camera_->lookAt(cameraTarget_);
  auto meshNode = std::make_unique<MeshNode>("Resources/BrainStem.glb", codeBook_,
                                             renderDevice_, this, camera_.get());
  meshNode->init();
  meshNode->setVisualizerSettings(visualizerSettings_);
  meshNode->setLocalPosition({0.0f, -1.0f, 0.0f});
  meshNode->setLocalScale({4.0f, 4.0f, 4.0f});
  meshNode_ = meshNode.get();
  addChild(std::move(meshNode));
}

void GltfNodeScene::update(const DL::FrameContext &ctx) {
  if (meshNode_ != nullptr) {
    const glm::quat baseRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    meshNode_->setLocalRotation(baseRotation);
  }
  SceneNode::update(ctx);
}

void GltfNodeScene::render(const DL::FrameContext &ctx) {
  if (renderDevice_ != nullptr) {
    renderDevice_->beginFrame({.clearColor = {0.93f, 0.92f, 0.87f, 1.0f},
                               .clearFlags = DL::ClearFlags::ColorDepth,
                               .depthMode = DL::DepthMode::Less});
  }

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("glTF Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  if (ImGui::Checkbox("Debug normals", &debugNormals_) && meshNode_ != nullptr) {
    meshNode_->setDebugNormals(debugNormals_);
  }
  if (meshNode_ != nullptr && meshNode_->hasAnimations()) {
    bool playAnimation = meshNode_->isAnimationPlaying();
    if (ImGui::Checkbox("Play animation", &playAnimation)) {
      meshNode_->setAnimationPlaying(playAnimation);
    }
    float playbackSpeed = meshNode_->animationPlaybackSpeed();
    if (ImGui::SliderFloat("Playback speed", &playbackSpeed, 0.0f, 2.0f)) {
      meshNode_->setAnimationPlaybackSpeed(playbackSpeed);
    }
  }
  bool changed = false;
  if (camera_ != nullptr) {
    auto cameraPosition = camera_->getPosition();
    bool cameraChanged = false;
    cameraChanged |=
        ImGui::SliderFloat3("Camera pos", &cameraPosition.x, -20.0f, 20.0f);
    cameraChanged |=
        ImGui::SliderFloat3("Camera target", &cameraTarget_.x, -10.0f, 10.0f);
    cameraChanged |= ImGui::SliderFloat("Camera FOV", &camera_->mFov, 20.0f, 90.0f);
    if (cameraChanged) {
      camera_->setPosition(cameraPosition);
      camera_->lookAt(cameraTarget_);
    }
  }
  changed |= ImGui::SliderFloat3("Light dir", &visualizerSettings_.lightDirection.x,
                                 -1.0f, 1.0f);
  changed |= ImGui::ColorEdit3("Light color", &visualizerSettings_.lightColor.x);
  changed |= ImGui::SliderFloat("Ambient", &visualizerSettings_.ambientStrength,
                                0.0f, 1.5f);
  changed |= ImGui::SliderFloat("Specular", &visualizerSettings_.specularStrength,
                                0.0f, 1.0f);
  changed |= ImGui::SliderFloat("Shininess", &visualizerSettings_.shininess,
                                1.0f, 64.0f);
  if (changed && meshNode_ != nullptr) {
    meshNode_->setVisualizerSettings(visualizerSettings_);
  }
  ImGui::End();
#endif

  SceneNode::render(ctx);
}

void GltfNodeScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

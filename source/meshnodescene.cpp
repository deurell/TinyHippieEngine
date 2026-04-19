#include "meshnodescene.h"

#include "debugui.h"
#ifdef USE_IMGUI
#include "imgui.h"
#endif

MeshNodeScene::MeshNodeScene(DL::IRenderDevice *renderDevice,
                             basist::etc1_global_selector_codebook *codeBook,
                             DL::MeshAssetCache *meshAssetCache,
                             DL::RenderResourceCache *renderResourceCache)
    : SceneNode(nullptr), renderDevice_(renderDevice), codeBook_(codeBook),
      meshAssetCache_(meshAssetCache), renderResourceCache_(renderResourceCache) {}

void MeshNodeScene::init() {
  SceneNode::init();

  auto meshNode = std::make_unique<MeshNode>("Resources/bridge.obj", codeBook_,
                                             renderDevice_, meshAssetCache_,
                                             renderResourceCache_, this);
  meshNode->init();
  meshNode->setVisualizerSettings(visualizerSettings_);
  meshNode->setLocalPosition({0.0f, -4.0f, 0.0f});
  meshNode->setLocalScale({2.8f, 2.8f, 2.8f});
  meshNode_ = meshNode.get();
  addChild(std::move(meshNode));
}

void MeshNodeScene::update(const DL::FrameContext &ctx) {
  if (meshNode_ != nullptr) {
    meshNode_->setLocalRotation(
        glm::angleAxis(static_cast<float>(ctx.total_time) * 0.15f,
                       glm::vec3(0.0f, 1.0f, 0.0f)));
  }
  SceneNode::update(ctx);
}

void MeshNodeScene::render(const DL::FrameContext &ctx) {
  if (renderDevice_ != nullptr) {
    renderDevice_->beginFrame({.clearColor = {0.70f, 0.90f, 1.00f, 1.0f},
                               .clearFlags = DL::ClearFlags::ColorDepth,
                               .depthMode = DL::DepthMode::Less});
  }

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("Mesh Node Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  if (ImGui::Checkbox("Debug normals", &debugNormals_) && meshNode_ != nullptr) {
    meshNode_->setDebugNormals(debugNormals_);
  }
  bool changed = false;
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

void MeshNodeScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

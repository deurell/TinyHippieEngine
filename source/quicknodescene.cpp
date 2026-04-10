#include "quicknodescene.h"
#include "debugui.h"
#include "imgui.h"
#include "planenode.h"
#include <algorithm>
#include <memory>
#include <ranges>

QuickNodeScene::QuickNodeScene(std::string_view glslVersionString,
                               DL::IRenderDevice *renderDevice)
    : glslVersionString_(glslVersionString), renderDevice_(renderDevice) {}

void QuickNodeScene::init() {
  SceneNode::init();
  camera_ = std::make_unique<DL::Camera>(glm::vec3(0, 0, 46));
  for (unsigned int i = 0; i < number_of_planes; i++) {
    addChild(createPlane(camera_.get()));
  }
}

void QuickNodeScene::update(const DL::FrameContext &ctx) {
  SceneNode::update(ctx);
  bounce(ctx.total_time);
}

void QuickNodeScene::render(const DL::FrameContext &ctx) {
  if (renderDevice_ != nullptr) {
    renderDevice_->beginFrame({.clearColor = {0.0f, 0.0f, 0.0f, 1.0f},
                               .clearFlags = DL::ClearFlags::ColorDepth,
                               .depthMode = DL::DepthMode::Less});
  }

  SceneNode::render(ctx);

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("Quick Node Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::SliderFloat("f1", &f1, 0.0f, 2.0f);
  ImGui::SliderFloat("f2", &f2, 0.0f, 2.0f);
  ImGui::SliderFloat("f3", &f3, 0.0f, 2.0f);
  ImGui::SliderFloat("fAmp", &fAmp, 0.0f, 100.0f);
  ImGui::SliderFloat("fOffset", &fOffset, 0.0f, 2.0f);
  ImGui::End();
#endif
}

void QuickNodeScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

void QuickNodeScene::bounce(double totalTime) const {
  float offset = glm::radians(0.0f);
  auto time = static_cast<float>(totalTime);
  for (auto &child : children) {
    auto *node = dynamic_cast<PlaneNode *>(child.get());
    float x = 7.5f * sinf(f1 * time + offset);
    float y = 8.9f * sinf(f2 * -time + offset);
    float z = fAmp * sinf(f3 * time + offset);
    node->setLocalPosition({x, y, z});
    float col = (z + fAmp) / (2.0f * fAmp);
    node->color = {col, col, 1.0, 1.0f};
    offset += fOffset;
  }
}

std::unique_ptr<PlaneNode> QuickNodeScene::createPlane(DL::Camera *camera) {
  auto planeNode =
      std::make_unique<PlaneNode>(glslVersionString_, this, camera, renderDevice_);
  planeNode->init();
  return planeNode;
}

//
// Created by Mikael Deurell on 2021-12-15.
//

#include "simplescene.h"
#include "debugui.h"
#ifdef USE_IMGUI
#include "imgui.h"
#endif
#include <iostream>

SimpleScene::SimpleScene(DL::IRenderDevice *renderDevice)
    : renderDevice_(renderDevice) {}

SimpleScene::~SimpleScene() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid()) {
      renderDevice_->destroy(mesh_);
    }
    if (pipeline_.valid()) {
      renderDevice_->destroy(pipeline_);
    }
  }
}

void SimpleScene::init() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});
  if (renderDevice_ == nullptr) {
    return;
  }
  pipeline_ =
      renderDevice_->createPipeline("Shaders/simple.vert", "Shaders/simple.frag");
  mesh_ = renderDevice_->createTexturedQuad();
}

void SimpleScene::update(const DL::FrameContext & /*ctx*/) {}

void SimpleScene::render(const DL::FrameContext &ctx) {
  if (renderDevice_ != nullptr) {
    renderDevice_->beginFrame({.clearColor = {0.0f, 0.0f, 0.0f, 1.0f},
                               .clearFlags = DL::ClearFlags::Color,
                               .depthMode = DL::DepthMode::Disabled});
  }

  if (renderDevice_ != nullptr && mesh_.valid() && pipeline_.valid()) {
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = mCamera->getViewMatrix();
    glm::mat4 projection = mCamera->getPerspectiveTransform();

    DL::DrawCommand command;
    command.mesh = mesh_;
    command.pipeline = pipeline_;
    command.uniforms.push_back(
        DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
    command.uniforms.push_back(
        DL::UniformValue::makeVec4("baseColor", {0.2f, 0.7f, 1.0f, 1.0f}));
    command.uniforms.push_back(DL::UniformValue::makeMat4("model", model));
    command.uniforms.push_back(DL::UniformValue::makeMat4("view", view));
    command.uniforms.push_back(
        DL::UniformValue::makeMat4("projection", projection));
    renderDevice_->draw(command);
  }

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("simple scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif
}

void SimpleScene::onClick(double x, double y) {
  std::cout << "Mouse click @ x:" << x << " y:" << y << std::endl;
}

void SimpleScene::onKey(int key) { std::cout << key << std::endl; }

void SimpleScene::onScreenSizeChanged(glm::vec2 size) {
  mScreenSize = size;
  mCamera->mScreenSize = size;
}

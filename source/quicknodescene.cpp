#include "quicknodescene.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "planenode.h"
#include <algorithm>
#include <memory>
#include <ranges>

QuickNodeScene::QuickNodeScene(std::string_view glslVersionString)
    : glslVersionString_(glslVersionString) {}

void QuickNodeScene::init() {
  SceneNode::init();
  camera_ = std::make_unique<DL::Camera>(glm::vec3(0, 0, 46));
  for (uint i = 0; i < number_of_planes; i++) {
    addChild(createPlane(camera_.get()));
  }
}

void QuickNodeScene::update(float delta) {
  SceneNode::update(delta);
  bounce(delta);
}

void QuickNodeScene::render(float delta) {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  SceneNode::render(delta);

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
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

void QuickNodeScene::bounce(float delta) const {
  float offset = glm::radians(0.0f);
  for (auto &child : children) {
    auto *node = dynamic_cast<PlaneNode *>(child.get());
    float x = 7.5f * sinf(f1 * glfwGetTime() + offset);
    float y = 8.9f * sinf(f2 * -glfwGetTime() + offset);
    float z = fAmp * sinf(f3 * glfwGetTime() + offset);
    node->setLocalPosition({x, y, z});
    float col = (z + fAmp) / (2.0f * fAmp);
    node->color = {col, col, 1.0, 1.0f};
    offset += fOffset;
  }
}

std::unique_ptr<PlaneNode> QuickNodeScene::createPlane(DL::Camera *camera) {
  auto planeNode =
      std::make_unique<PlaneNode>(glslVersionString_, this, camera);
  planeNode->init();
  return planeNode;
}
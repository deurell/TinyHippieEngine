#include "quicknodescene.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "planenode.h"
#include <memory>

QuickNodeScene::QuickNodeScene(std::string_view glslVersionString)
    : glslVersionString_(glslVersionString) {}

void QuickNodeScene::init() {
  SceneNode::init();
  camera_ = std::make_unique<DL::Camera>(glm::vec3(0, 0, 16));
  auto plane =
      std::make_unique<PlaneNode>(glslVersionString_, this, camera_.get());
  plane->planeType = PlaneNode::PlaneType::Simple;
  plane->color = {0.5f, 0.5f, 1.0f, 1.0f};
  plane->init();
  plane->setLocalPosition({0, 0, 0});
  plane_ = plane.get();
  addChild(std::move(plane));
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
  ImGui::End();
#endif
}

void QuickNodeScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}


void QuickNodeScene::bounce(float delta) {
  float time = glfwGetTime();
  glm::vec3 deltaPosition{2.5f * sinf(time * 1.5f),
                          1.9f * sinf(time * 1.3f),
                          abs(4.9f * sinf(time * 1.8f))};
  plane_->setLocalPosition(deltaPosition);
  float col = plane_->getLocalPosition().z / 5.0f;
  plane_->color = {col, col, 1.0f, 1.0f};
}
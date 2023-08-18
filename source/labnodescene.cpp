//
// Created by Mikael Deurell on 2023-08-18.
//
#include "labnodescene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "simplenode.h"

LabNodeScene::LabNodeScene(std::string_view glslVersionString) {
  glslVersionString_ = glslVersionString;
}

void LabNodeScene::init() {
  SceneNode::init();
  setLocalPosition({-12, 0, 0});

  auto simpleNode = std::make_unique<SimpleNode>(glslVersionString_);
  simpleNode->init();
  simpleNode->setLocalPosition({12, 0, 0});
  simpleNode->dirty = true;
  children.emplace_back(std::move(simpleNode));
  }

void LabNodeScene::render(float delta) {
#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("Node Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif
  SceneNode::render(delta);
}

void LabNodeScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

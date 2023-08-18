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
  setLocalPosition({-6, 0, 0});

  auto planeNode = std::make_unique<PlaneNode>(glslVersionString_);
  planeNode->init();
  planeNode->setLocalPosition({6, 0, 0});
  glm::quat rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 0, 1));
  planeNode->setLocalRotation(rotation);
  planeNode->setLocalScale({2, 6, 1});
  children.emplace_back(std::move(planeNode));
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

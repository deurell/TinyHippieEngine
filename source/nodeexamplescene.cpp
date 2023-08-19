//
// Created by Mikael Deurell on 2023-08-18.
//
#include "nodeexamplescene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "planenode.h"

NodeExampleScene::NodeExampleScene(std::string_view glslVersionString) {
  glslVersionString_ = glslVersionString;
}

void NodeExampleScene::init() {
  SceneNode::init();
  setLocalPosition({0, 0, 0});

  auto planeNode = std::make_unique<PlaneNode>(glslVersionString_);
  planeNode->init();
  planeNode->setLocalPosition({6, 0, 0});
  glm::quat rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 0, 1));
  planeNode->setLocalRotation(rotation);
  planeNode->setLocalScale({4, 2, 1});
  children.emplace_back(std::move(planeNode));

  auto planeNode2 = std::make_unique<PlaneNode>(glslVersionString_);
  planeNode2->init();
  planeNode2->setLocalPosition({-6, 0, 0});
  glm::quat rotation2 = glm::angleAxis(glm::radians(135.0f), glm::vec3(0, 0, 1));
  planeNode2->setLocalRotation(rotation);
  planeNode2->setLocalScale({2, 4, 1});
  children.emplace_back(std::move(planeNode2));

}

void NodeExampleScene::render(float delta) {
#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("Node Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  SceneNode::render(delta);
}

void NodeExampleScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

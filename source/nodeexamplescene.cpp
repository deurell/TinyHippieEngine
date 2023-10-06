//
// Created by Mikael Deurell on 2023-08-18.
//
#include "nodeexamplescene.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/constants.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "planenode.h"
#include <memory>
#include "textnode.h"

NodeExampleScene::NodeExampleScene(std::string_view glslVersionString)
    : SceneNode(nullptr), glslVersionString_(glslVersionString) {}

void NodeExampleScene::init() {
  SceneNode::init();
  setLocalPosition({0, 0, 0});

  auto planeNode = std::make_unique<PlaneNode>(glslVersionString_);
  planeNode->init();
  planeNode->setLocalPosition({6, 0, 0});
  glm::quat rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 0, 1));
  planeNode->setLocalRotation(rotation);
  planeNode->setLocalScale({4, 2, 1});
  plane2_ = planeNode.get();
  addChild(std::move(planeNode));

  auto planeNode2 = std::make_unique<PlaneNode>(glslVersionString_);
  planeNode2->init();
  planeNode2->setLocalPosition({-6, 0, 0});
  glm::quat rotation2 = glm::angleAxis(glm::radians(10.0f), glm::vec3(0, 0, 1));
  planeNode2->setLocalRotation(rotation2);
  planeNode2->setLocalScale({2, 4, 1});
  plane2_ = planeNode2.get();
  addChild(std::move(planeNode2));

  auto textNode = std::make_unique<TextNode>(glslVersionString_, this, "Hello\nWorld!");
  textNode->init();
  textNode->setLocalPosition({-5, 5, 0});
  textNode->setLocalScale({1.0, 1.0, 1});
  textNode->setLocalRotation(glm::angleAxis(-45.0f, glm::vec3(0, 0, 1)));
  textNode_ = textNode.get();
  addChild(std::move(textNode));
}

void NodeExampleScene::update(float delta) {
  float time = glfwGetTime();
  float rotation = sin(time * 5.0) * 0.6f;
  float scale_ = 1.4f + sin(time * 2.5) * 0.4f;
  plane2_->setLocalScale({scale_, scale_, 1});
  plane2_->setLocalRotation(glm::angleAxis(rotation, glm::vec3(0, 0, 1)));
  float textRotation = sin(time * 1.5) * glm::pi<float>();
  textNode_->setLocalRotation(glm::angleAxis(textRotation, glm::vec3(1, 0, 0)));
  SceneNode::update(delta);
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

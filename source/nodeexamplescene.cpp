//
// Created by Mikael Deurell on 2023-08-18.
//
#include "nodeexamplescene.h"
#include "GLFW/glfw3.h"
#include "glm/ext/scalar_constants.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "planenode.h"
#include "textnode.h"
#include "textvisualizer.h"

NodeExampleScene::NodeExampleScene(std::string glslVersionString)
    : SceneNode(nullptr), glslVersionString_(std::move(glslVersionString)) {}

void NodeExampleScene::init() {
  SceneNode::init();
  setLocalPosition({0, 0, 0});

  auto plane =
      createPlane({6, 4, 0}, {4, 2, 1},
                  glm::angleAxis(glm::radians(10.0f), glm::vec3(0, 0, 1)));
  plane1_ = plane.get();
  addChild(std::move(plane));

  auto plane2 =
      createPlane({-6, 4, 0}, {2, 4, 1},
                  glm::angleAxis(glm::radians(10.0f), glm::vec3(0, 0, 1)));
  plane2_ = plane2.get();
  addChild(std::move(plane2));

  std::string text = R"(
A long time ago,
in a galaxy far, far away...
It is a period of civil war. Rebel
spaceships, striking from a hidden
base, have won their first victory
against the evil Galactic Empire.

During the battle, Rebel spies managed
to steal secret plans to the Empire's
ultimate weapon, the Death Star, an
armored space station with enough
power to destroy an entire planet.

Pursued by the Empire's sinister agents,
Princess Leia races home aboard her
starship, custodian of the stolen plans
that can save her people and restore
freedom to the galaxy...
  )";

  auto textNode = std::make_unique<TextNode>(glslVersionString_, this, text);
  textNode->init();
  textNode->setLocalPosition(INITIAL_TEXT_POSITION);
  textNode->setLocalScale({.1f, .1f, 1});
  textNode->setLocalRotation(glm::angleAxis(scrollAngle, glm::vec3(1, 0, 0)));
  textNode_ = textNode.get();
  addChild(std::move(textNode));

  textNode_->getCamera().setPosition({0, -40, 40});
  textNode_->getCamera().lookAt({0, 0, 0});
}

void NodeExampleScene::update(float delta) {
  float time = glfwGetTime();
  float rotation = sin(time * 5.0) * 0.6f;
  float scale_ = 1.4f + sin(time * 2.5) * 0.4f;
  if (plane2_) {
    plane2_->setLocalScale({scale_, scale_, 1});
    plane2_->setLocalRotation(glm::angleAxis(rotation, glm::vec3(0, 0, 1)));
  }

  float rotation2 = cos(time * 1.5) * glm::pi<float>();
  if (plane1_) {
    plane1_->setLocalRotation(glm::angleAxis(rotation2, glm::vec3(0, 1, 0)));
  }

  const float theta = glm::radians(scrollAngle);
  float delta_y = scrollSpeed * cos(theta);
  float delta_z = scrollSpeed * sin(theta);
  auto position = textNode_->getLocalPosition();
  textNode_->setLocalPosition(position + glm::vec3(0, delta_y, -delta_z));

  if (textNode_->getLocalPosition().y >= TEXT_RESET_POSITION) {
    wrapScrollText();
  }

  SceneNode::update(delta);
}

void NodeExampleScene::render(float delta) {
  auto *textVisualizer =
      dynamic_cast<DL::TextVisualizer *>(textNode_->getVisualizer("main"));
#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("Node Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("scrollPosition: %.1f", textNode_->getLocalPosition().y);
  ImGui::SliderFloat("spd", &scrollSpeed, -2.0f, 2.0f);
  ImGui::SliderFloat("a1", &textVisualizer->rotAngle1_, -1.0f, 1.0f);
  ImGui::SliderFloat("a2", &textVisualizer->rotAngle2_, -1.0f, 1.0f);
  ImGui::SliderFloat("c1", &textVisualizer->color1_, 0.0f, 0.2f);
  ImGui::SliderFloat("c2", &textVisualizer->color2_, -4.0f, 4.0f);
  if (ImGui::Button("wrap")) {
    wrapScrollText();
  }
  ImGui::End();
#endif

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  SceneNode::render(delta);
}

void NodeExampleScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

std::unique_ptr<PlaneNode> NodeExampleScene::createPlane(glm::vec3 position,
                                                         glm::vec3 scale,
                                                         glm::quat rotation) {
  auto planeNode = std::make_unique<PlaneNode>(glslVersionString_);
  planeNode->init();
  planeNode->setLocalPosition(position);
  planeNode->setLocalRotation(rotation);
  planeNode->setLocalScale(scale);
  return planeNode;
}

void NodeExampleScene::wrapScrollText() {
  textNode_->setLocalPosition(INITIAL_TEXT_POSITION);
}

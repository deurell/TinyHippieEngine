//
// Created by Mikael Deurell on 2024-08-22.
//
#include "glosifyscene.h"

#include "imagenode.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "planenode.h"

DL::GlosifyScene::GlosifyScene(std::string glslVersionString,
                               basist::etc1_global_selector_codebook *codeBook)
    : SceneNode(nullptr), glslVersionString_(std::move(glslVersionString)),
      codeBook_(codeBook) {}

void DL::GlosifyScene::init() {
  SceneNode::init();
  setLocalPosition({0, 0, 0});

  auto plane =
      createPlane({0, 0, 0}, {16, 16, 1},
                  glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1)));
  plane_node_ = plane.get();
  addChild(std::move(plane));

  auto imageNode =
      createImage({0, 0, 10}, {3.0, 2.0, 1.0},
                  glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1)));
  image_node_ = imageNode.get();
  addChild(std::move(imageNode));
}

void DL::GlosifyScene::update(float delta) { SceneNode::update(delta); }

void DL::GlosifyScene::render(float delta) {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("Glosify Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif

  SceneNode::render(delta);
}

void DL::GlosifyScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

std::unique_ptr<PlaneNode> DL::GlosifyScene::createPlane(glm::vec3 position,
                                                         glm::vec3 scale,
                                                         glm::quat rotation) {
  auto planeNode = std::make_unique<PlaneNode>(glslVersionString_);
  planeNode->planeType = PlaneNode::PlaneType::Spinner;
  planeNode->init();
  planeNode->setLocalPosition(position);
  planeNode->setLocalRotation(rotation);
  planeNode->setLocalScale(scale);
  return planeNode;
}

std::unique_ptr<ImageNode> DL::GlosifyScene::createImage(glm::vec3 position,
                                                         glm::vec3 scale,
                                                         glm::quat rotation) {
  auto imageNode = std::make_unique<ImageNode>(
      glslVersionString_, "Resources/sup.basis", codeBook_);
  imageNode->init();
  imageNode->setLocalPosition(position);
  imageNode->setLocalRotation(rotation);
  imageNode->setLocalScale(scale);
  return imageNode;
}

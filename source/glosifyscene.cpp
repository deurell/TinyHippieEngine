//
// Created by Mikael Deurell on 2024-08-22.
//
#include "glosifyscene.h"
#include "debugui.h"
#include "imgui.h"
#include "planenode.h"
#include "spritenode.h"

DL::GlosifyScene::GlosifyScene(basist::etc1_global_selector_codebook *codeBook,
                               DL::IRenderDevice *renderDevice,
                               DL::RenderResourceCache *renderResourceCache)
    : SceneNode(nullptr), codeBook_(codeBook), renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache) {}

void DL::GlosifyScene::init() {
  SceneNode::init();
  setLocalPosition({0, 0, 0});

  auto plane =
      createPlane({0, 0, 0}, {16, 16, 1},
                  glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1)));
  plane_node_ = plane.get();
  addChild(std::move(plane));

  auto spriteNode =
      createSprite({0, 0, 10}, {3.0, 2.0, 1.0},
                   glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1)));
  sprite_node_ = spriteNode.get();
  addChild(std::move(spriteNode));
}

void DL::GlosifyScene::update(const DL::FrameContext &ctx) { SceneNode::update(ctx); }

void DL::GlosifyScene::render(const DL::FrameContext &ctx) {
  if (renderDevice_ != nullptr) {
    renderDevice_->beginFrame({.clearColor = {0.0f, 0.0f, 0.0f, 1.0f},
                               .clearFlags = DL::ClearFlags::ColorDepth,
                               .depthMode = DL::DepthMode::Less});
  }

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("Glosify Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif

  SceneNode::render(ctx);
}

void DL::GlosifyScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

std::unique_ptr<PlaneNode> DL::GlosifyScene::createPlane(glm::vec3 position,
                                                         glm::vec3 scale,
                                                         glm::quat rotation) {
  auto planeNode = std::make_unique<PlaneNode>(nullptr, nullptr, renderDevice_,
                                               renderResourceCache_);
  planeNode->planeType = PlaneNode::PlaneType::Spinner;
  planeNode->init();
  planeNode->setLocalPosition(position);
  planeNode->setLocalRotation(rotation);
  planeNode->setLocalScale(scale);
  return planeNode;
}

std::unique_ptr<SpriteNode> DL::GlosifyScene::createSprite(glm::vec3 position,
                                                           glm::vec3 scale,
                                                           glm::quat rotation) {
  auto spriteNode = std::make_unique<SpriteNode>("Resources/sup.basis",
                                                codeBook_, renderDevice_,
                                                renderResourceCache_);
  spriteNode->init();
  spriteNode->setLocalPosition(position);
  spriteNode->setLocalRotation(rotation);
  spriteNode->setLocalScale(scale);
  return spriteNode;
}

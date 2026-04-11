#include "phongshapescene.h"

#include "debugui.h"
#include "imgui.h"

PhongShapeScene::PhongShapeScene(DL::IRenderDevice *renderDevice)
    : SceneNode(nullptr), renderDevice_(renderDevice) {}

void PhongShapeScene::init() {
  SceneNode::init();
  camera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 1.5f, 9.0f));
  camera_->lookAt({0.0f, 0.0f, 0.0f});

  auto cube = std::make_unique<PhongShapeNode>(ShapeType::Cube, renderDevice_, this,
                                               camera_.get());
  cube->setMaterial({.diffuse = {0.96f, 0.38f, 0.24f},
                     .ambient = {0.35f, 0.18f, 0.14f},
                     .specular = {0.35f, 0.35f, 0.35f},
                     .shininess = 20.0f});
  cube->init();
  cube->setLocalPosition({-2.4f, 0.0f, 0.0f});
  cubeNode_ = cube.get();
  addChild(std::move(cube));

  auto sphere = std::make_unique<PhongShapeNode>(ShapeType::Sphere, renderDevice_, this,
                                                 camera_.get());
  sphere->setMaterial({.diffuse = {0.2f, 0.7f, 0.92f},
                       .ambient = {0.14f, 0.24f, 0.32f},
                       .specular = {0.55f, 0.55f, 0.6f},
                       .shininess = 28.0f});
  sphere->init();
  sphere->setLocalPosition({0.0f, 0.0f, 0.0f});
  sphereNode_ = sphere.get();
  addChild(std::move(sphere));

  auto cylinder = std::make_unique<PhongShapeNode>(ShapeType::Cylinder, renderDevice_, this,
                                                   camera_.get());
  cylinder->setMaterial({.diffuse = {0.75f, 0.84f, 0.28f},
                         .ambient = {0.25f, 0.3f, 0.14f},
                         .specular = {0.25f, 0.25f, 0.2f},
                         .shininess = 12.0f});
  cylinder->init();
  cylinder->setLocalPosition({2.4f, 0.0f, 0.0f});
  cylinderNode_ = cylinder.get();
  addChild(std::move(cylinder));
}

void PhongShapeScene::update(const DL::FrameContext &ctx) {
  if (cubeNode_ != nullptr) {
    cubeNode_->setLocalRotation(glm::angleAxis(
        static_cast<float>(ctx.total_time) * 0.6f, glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f))));
  }
  if (sphereNode_ != nullptr) {
    sphereNode_->setLocalRotation(glm::angleAxis(
        static_cast<float>(ctx.total_time) * 0.4f, glm::vec3(0.0f, 1.0f, 0.0f)));
  }
  if (cylinderNode_ != nullptr) {
    cylinderNode_->setLocalRotation(glm::angleAxis(
        static_cast<float>(ctx.total_time) * -0.7f, glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f))));
  }
  SceneNode::update(ctx);
}

void PhongShapeScene::render(const DL::FrameContext &ctx) {
  if (renderDevice_ != nullptr) {
    renderDevice_->beginFrame({.clearColor = {0.07f, 0.09f, 0.12f, 1.0f},
                               .clearFlags = DL::ClearFlags::ColorDepth,
                               .depthMode = DL::DepthMode::Less});
  }

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("Phong Shapes");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

  ImGui::End();
#endif

  SceneNode::render(ctx);
}

void PhongShapeScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

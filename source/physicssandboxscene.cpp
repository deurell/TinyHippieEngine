#include "physicssandboxscene.h"

#include "debugui.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

PhysicsSandboxScene::PhysicsSandboxScene(
    DL::IRenderDevice *renderDevice,
    DL::RenderResourceCache *renderResourceCache)
    : SceneNode(nullptr), renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache) {}

void PhysicsSandboxScene::init() {
  SceneNode::init();

  camera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 4.0f, 16.0f));
  camera_->lookAt({0.0f, 2.0f, 0.0f});
  physicsWorld_.setGravity({0.0f, -9.81f, 0.0f});

  auto *floor = addShape(ShapeType::Cube, {0.0f, -1.0f, 0.0f},
                         {12.0f, 1.0f, 12.0f},
                         {.diffuse = {0.32f, 0.34f, 0.38f},
                          .ambient = {0.18f, 0.18f, 0.2f},
                          .specular = {0.08f, 0.08f, 0.08f},
                          .shininess = 6.0f});
  addPhysicsBody(*floor, {.type = DL::PhysicsBodyType::Static,
                          .shape = DL::PhysicsShapeDesc::makeBox(
                              {6.0f, 0.5f, 6.0f}),
                          .position = floor->getLocalPosition()});

  auto *cube = addShape(ShapeType::Cube, {-2.25f, 6.0f, 0.0f},
                        {1.2f, 1.2f, 1.2f},
                        {.diffuse = {0.96f, 0.42f, 0.22f},
                         .ambient = {0.28f, 0.16f, 0.12f},
                         .specular = {0.25f, 0.25f, 0.25f},
                         .shininess = 20.0f});
  addPhysicsBody(*cube, {.type = DL::PhysicsBodyType::Dynamic,
                         .shape = DL::PhysicsShapeDesc::makeBox(
                             {0.6f, 0.6f, 0.6f}),
                         .position = cube->getLocalPosition()});

  auto *sphere = addShape(ShapeType::Sphere, {2.25f, 8.0f, 0.0f},
                          {1.0f, 1.0f, 1.0f},
                          {.diffuse = {0.22f, 0.7f, 0.94f},
                           .ambient = {0.14f, 0.22f, 0.28f},
                           .specular = {0.4f, 0.45f, 0.5f},
                           .shininess = 28.0f});
  addPhysicsBody(*sphere, {.type = DL::PhysicsBodyType::Dynamic,
                           .shape = DL::PhysicsShapeDesc::makeSphere(0.5f),
                           .position = sphere->getLocalPosition()});

  raycastMarkerNode_ = addShape(ShapeType::Sphere, {0.0f, -1000.0f, 0.0f},
                                {0.18f, 0.18f, 0.18f},
                                {.diffuse = {0.1f, 1.0f, 0.3f},
                                 .ambient = {0.05f, 0.3f, 0.1f},
                                 .specular = {0.2f, 0.2f, 0.2f},
                                 .shininess = 12.0f});
}

void PhysicsSandboxScene::update(const DL::FrameContext &ctx) {
  fixedStepsLastFrame_ = fixedStepsCurrentFrame_;
  fixedStepsCurrentFrame_ = 0;
  SceneNode::update(ctx);
}

void PhysicsSandboxScene::fixedUpdate(const DL::FrameContext &ctx) {
  if (paused_) {
    return;
  }

  // Refresh world transforms before pushing node transforms into physics.
  SceneNode::update({0.0f, ctx.total_time});

  for (auto &binding : bodyBindings_) {
    if (binding.body != nullptr) {
      binding.body->syncBeforeStep();
    }
  }

  physicsWorld_.step(ctx.delta_time);

  for (auto &binding : bodyBindings_) {
    if (binding.body != nullptr) {
      binding.body->syncAfterStep();
    }
  }

  fixedStepsCurrentFrame_ += 1;
}

void PhysicsSandboxScene::render(const DL::FrameContext &ctx) {
  if (renderDevice_ != nullptr) {
    renderDevice_->beginFrame({.clearColor = {0.08f, 0.1f, 0.13f, 1.0f},
                               .clearFlags = DL::ClearFlags::ColorDepth,
                               .depthMode = DL::DepthMode::Less});
  }

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("Physics Sandbox");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Checkbox("Pause simulation", &paused_);
  const char *spawnShapeLabels[] = {"Box", "Sphere", "Capsule"};
  int spawnShapeIndex = static_cast<int>(spawnShape_);
  if (ImGui::Combo("Spawn shape", &spawnShapeIndex, spawnShapeLabels,
                   IM_ARRAYSIZE(spawnShapeLabels))) {
    spawnShape_ = static_cast<SpawnShape>(spawnShapeIndex);
  }
  const bool canSpawnAtHit = lastRaycastHit_.hasHit;
  if (ImGui::Button("Spawn at hit") && canSpawnAtHit) {
    spawnDynamicBody(spawnShape_, lastRaycastHit_.point + lastRaycastHit_.normal);
  }
  if (!canSpawnAtHit) {
    ImGui::TextUnformatted("Click the scene to pick a spawn point");
  }
  if (ImGui::Button("Reset bodies")) {
    resetDynamicBodies();
  }
  ImGui::Text("Fixed steps last frame: %d", fixedStepsLastFrame_);
  if (lastRaycastHit_.hasHit) {
    ImGui::Text("Raycast hit: %.2f %.2f %.2f", lastRaycastHit_.point.x,
                lastRaycastHit_.point.y, lastRaycastHit_.point.z);
  } else {
    ImGui::TextUnformatted("Raycast hit: none");
  }
  ImGui::TextUnformatted("Click to raycast");
  ImGui::TextUnformatted("1/2/3 change spawn shape");
  ImGui::TextUnformatted("R resets bodies");
  ImGui::End();
#endif

  SceneNode::render(ctx);
}

void PhysicsSandboxScene::onClick(double x, double y) {
  const glm::vec3 rayStart = screenToWorld(x, y, 0.0f);
  const glm::vec3 rayEnd = screenToWorld(x, y, 1.0f);
  lastRaycastHit_ = physicsWorld_.raycast(rayStart, rayEnd);
  updateRaycastMarker();
}

void PhysicsSandboxScene::onKey(int key) {
  if (key == GLFW_KEY_R) {
    resetDynamicBodies();
  } else if (key == GLFW_KEY_1) {
    spawnShape_ = SpawnShape::Box;
  } else if (key == GLFW_KEY_2) {
    spawnShape_ = SpawnShape::Sphere;
  } else if (key == GLFW_KEY_3) {
    spawnShape_ = SpawnShape::Capsule;
  } else if (key == GLFW_KEY_SPACE || key == GLFW_KEY_S) {
    const glm::vec3 spawnPosition =
        lastRaycastHit_.hasHit
            ? lastRaycastHit_.point + lastRaycastHit_.normal
            : defaultSpawnPosition(spawnShape_);
    spawnDynamicBody(spawnShape_, spawnPosition);
  }
}

void PhysicsSandboxScene::onScreenSizeChanged(glm::vec2 size) {
  screenSize_ = size;
  if (framebufferSize_.x == 0.0f || framebufferSize_.y == 0.0f) {
    framebufferSize_ = size;
  }
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = framebufferSize_;
  }
}

void PhysicsSandboxScene::onFramebufferSizeChanged(glm::vec2 size) {
  framebufferSize_ = size;
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

PhongShapeNode *PhysicsSandboxScene::addShape(
    ShapeType shapeType, const glm::vec3 &position, const glm::vec3 &scale,
    const DL::PhongMaterial &material) {
  auto node = std::make_unique<PhongShapeNode>(
      shapeType, renderDevice_, renderResourceCache_, this, camera_.get());
  node->setMaterial(material);
  node->setLocalPosition(position);
  node->setLocalScale(scale);
  node->init();
  auto *rawNode = node.get();
  addChild(std::move(node));
  rawNode->updateTransforms(getWorldTransform());
  return rawNode;
}

void PhysicsSandboxScene::addPhysicsBody(PhongShapeNode &node,
                                         const DL::PhysicsBodyDesc &bodyDesc) {
  auto body = std::make_unique<DL::PhysicsBodyComponent>(physicsWorld_, node,
                                                         bodyDesc);
  bodyBindings_.push_back(
      {.node = &node, .body = std::move(body), .spawnPosition = bodyDesc.position});
}

PhongShapeNode *PhysicsSandboxScene::spawnDynamicBody(SpawnShape shape,
                                                      const glm::vec3 &position) {
  ShapeType visualShape = ShapeType::Cube;
  glm::vec3 scale(1.0f);
  DL::PhysicsShapeDesc physicsShape;
  DL::PhongMaterial material{};

  switch (shape) {
  case SpawnShape::Box:
    visualShape = ShapeType::Cube;
    scale = {1.0f, 1.0f, 1.0f};
    physicsShape = DL::PhysicsShapeDesc::makeBox({0.5f, 0.5f, 0.5f});
    material = {.diffuse = {0.92f, 0.5f, 0.18f},
                .ambient = {0.24f, 0.14f, 0.1f},
                .specular = {0.25f, 0.25f, 0.25f},
                .shininess = 18.0f};
    break;
  case SpawnShape::Sphere:
    visualShape = ShapeType::Sphere;
    scale = {1.0f, 1.0f, 1.0f};
    physicsShape = DL::PhysicsShapeDesc::makeSphere(0.5f);
    material = {.diffuse = {0.2f, 0.72f, 0.95f},
                .ambient = {0.1f, 0.2f, 0.28f},
                .specular = {0.4f, 0.45f, 0.5f},
                .shininess = 24.0f};
    break;
  case SpawnShape::Capsule:
    visualShape = ShapeType::Cylinder;
    scale = {0.8f, 1.3f, 0.8f};
    physicsShape = DL::PhysicsShapeDesc::makeCapsule(0.4f, 0.8f);
    material = {.diffuse = {0.62f, 0.82f, 0.24f},
                .ambient = {0.2f, 0.26f, 0.1f},
                .specular = {0.25f, 0.25f, 0.2f},
                .shininess = 12.0f};
    break;
  }

  auto *node = addShape(visualShape, position, scale, material);
  addPhysicsBody(*node, {.type = DL::PhysicsBodyType::Dynamic,
                         .shape = physicsShape,
                         .position = position,
                         .categoryBits = 0x0002,
                         .maskBits = 0xFFFF});
  return node;
}

glm::vec3 PhysicsSandboxScene::defaultSpawnPosition(SpawnShape shape) const {
  switch (shape) {
  case SpawnShape::Box:
    return {-2.25f, 6.0f, 0.0f};
  case SpawnShape::Sphere:
    return {2.25f, 8.0f, 0.0f};
  case SpawnShape::Capsule:
    return {0.0f, 10.0f, 0.0f};
  }
  return {0.0f, 8.0f, 0.0f};
}

glm::vec3 PhysicsSandboxScene::screenToWorld(double x, double y,
                                             float depth) const {
  if (camera_ == nullptr || framebufferSize_.x <= 0.0f ||
      framebufferSize_.y <= 0.0f) {
    return {0.0f, 0.0f, 0.0f};
  }

  glm::vec2 scale(1.0f, 1.0f);
  if (screenSize_.x > 0.0f && screenSize_.y > 0.0f) {
    scale.x = framebufferSize_.x / screenSize_.x;
    scale.y = framebufferSize_.y / screenSize_.y;
  }

  const float mouseX = static_cast<float>(x) * scale.x;
  const float mouseY = static_cast<float>(y) * scale.y;
  const float viewportY = framebufferSize_.y - mouseY;
  const glm::vec4 viewport(0.0f, 0.0f, framebufferSize_.x, framebufferSize_.y);

  return glm::unProject(glm::vec3(mouseX, viewportY, depth),
                        camera_->getViewMatrix(),
                        camera_->getPerspectiveTransform(), viewport);
}

void PhysicsSandboxScene::updateRaycastMarker() {
  if (raycastMarkerNode_ == nullptr) {
    return;
  }

  if (lastRaycastHit_.hasHit) {
    raycastMarkerNode_->setLocalPosition(lastRaycastHit_.point);
  } else {
    raycastMarkerNode_->setLocalPosition({0.0f, -1000.0f, 0.0f});
  }
}

void PhysicsSandboxScene::resetDynamicBodies() {
  for (auto &binding : bodyBindings_) {
    if (binding.node == nullptr || binding.body == nullptr) {
      continue;
    }

    if (binding.spawnPosition.y <= 0.0f) {
      continue;
    }

    binding.node->setLocalPosition(binding.spawnPosition);
    binding.node->setLocalRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    binding.body->setTransform(binding.spawnPosition,
                               glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    binding.body->setLinearVelocity({0.0f, 0.0f, 0.0f});
  }

  lastRaycastHit_ = {};
  updateRaycastMarker();
}

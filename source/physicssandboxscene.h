#pragma once

#include "camera.h"
#include "physicsbodycomponent.h"
#include "physicsworld.h"
#include "phongshapenode.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class PhysicsSandboxScene : public DL::SceneNode {
public:
  explicit PhysicsSandboxScene(
      DL::IRenderDevice *renderDevice = nullptr,
      DL::RenderResourceCache *renderResourceCache = nullptr);
  ~PhysicsSandboxScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void fixedUpdate(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  void onFramebufferSizeChanged(glm::vec2 size) override;

private:
  struct BodyBinding {
    PhongShapeNode *node = nullptr;
    std::unique_ptr<DL::PhysicsBodyComponent> body;
    glm::vec3 spawnPosition{0.0f};
  };

  enum class SpawnShape {
    Box,
    Sphere,
    Capsule,
  };

  PhongShapeNode *addShape(ShapeType shapeType, const glm::vec3 &position,
                           const glm::vec3 &scale,
                           const DL::PhongMaterial &material);
  PhongShapeNode *spawnDynamicBody(SpawnShape shape, const glm::vec3 &position);
  [[nodiscard]] glm::vec3 defaultSpawnPosition(SpawnShape shape) const;
  glm::vec3 screenToWorld(double x, double y, float depth) const;
  void updateRaycastMarker();
  void addPhysicsBody(PhongShapeNode &node, const DL::PhysicsBodyDesc &bodyDesc);
  void resetDynamicBodies();

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::unique_ptr<DL::Camera> camera_;
  PhongShapeNode *raycastMarkerNode_ = nullptr;
  DL::PhysicsWorld physicsWorld_;
  std::vector<BodyBinding> bodyBindings_;
  bool paused_ = false;
  int fixedStepsLastFrame_ = 0;
  int fixedStepsCurrentFrame_ = 0;
  glm::vec2 screenSize_{0.0f, 0.0f};
  glm::vec2 framebufferSize_{0.0f, 0.0f};
  SpawnShape spawnShape_ = SpawnShape::Box;
  DL::PhysicsRaycastHit lastRaycastHit_;
};

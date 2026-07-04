#pragma once

#include "camera.h"
#include "physicscontext.h"
#include "physicsworld.h"
#include "renderdevice.h"
#include "scenenode.h"
#include <string_view>
#include <vector>

class PhysicsTestScene final : public DL::SceneNode {
public:
  explicit PhysicsTestScene(DL::IRenderDevice *renderDevice = nullptr);
  ~PhysicsTestScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void fixedUpdate(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  void onFramebufferSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "PhysicsTestScene";
  }

private:
  struct BodyRecord {
    DL::PhysicsBodyHandle handle;
    DL::PhysicsBodyType type = DL::PhysicsBodyType::Static;
    DL::SceneNode *node = nullptr;
    glm::vec3 spawnPosition{0.0f};
    glm::quat spawnRotation{1.0f, 0.0f, 0.0f, 0.0f};
  };

  struct PickState {
    bool hasRaycast = false;
    DL::PhysicsRaycastHit hit;
  };

  void initCamera();
  void initWorld();
  DL::PhysicsBodyHandle addBody(DL::PhysicsBodyDesc desc,
                                const glm::vec4 &color);
  void initHitMarker();
  void spawnBurst(int count);
  void spawnDynamicBody();
  void resetDynamicBodies();
  void syncVisualBodies();
  void updateCameraController(const DL::FrameContext &ctx);
  void pickPhysicsBody(glm::vec2 screenPoint);
  void updatePickMarker();
  void updateKinematicPusher(double totalTime);

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::PhysicsContext physicsContext_;
  DL::Camera camera_{glm::vec3(7.0f, 5.4f, 8.0f)};
  std::vector<BodyRecord> bodies_;
  std::vector<DL::PhysicsDebugLine> markerLines_;
  DL::PhysicsBodyHandle pusher_;
  DL::SceneNode *hitMarkerNode_ = nullptr;
  glm::vec3 cameraTarget_{0.0f, 0.6f, 0.0f};
  glm::vec2 windowSize_{0.0f};
  glm::vec2 framebufferSize_{0.0f};
  PickState pick_;
  float cameraYaw_ = 0.0f;
  float cameraPitch_ = 0.0f;
  bool simulationEnabled_ = true;
  bool debugLinesEnabled_ = false;
  bool velocityDebugEnabled_ = true;
  bool autoSpawnEnabled_ = false;
  float spawnAccumulator_ = 0.0f;
  int spawnCounter_ = 0;
  int resetCounter_ = 0;
  bool wasLeftMouseDown_ = false;
};

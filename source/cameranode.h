#pragma once

#include "camera.h"
#include "scenenode.h"
#include <optional>
#include <string_view>

namespace DL {

class CameraNode : public SceneNode {
public:
  explicit CameraNode(SceneNode *parentNode = nullptr);
  ~CameraNode() override = default;

  void init() override;
  void update(const FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "CameraNode";
  }

  Camera &camera() { return camera_; }
  const Camera &camera() const { return camera_; }

  void setFov(float fov);
  [[nodiscard]] float fov() const { return camera_.mFov; }
  void setActive(bool active) { active_ = active; }
  [[nodiscard]] bool active() const { return active_; }
  void setLookAtTarget(glm::vec3 target) { lookAtTarget_ = target; }
  void lookAtWorld(glm::vec3 target);
  void translateLocal(glm::vec3 offset);
  void syncCameraFromTransform();

private:
  void applyLookAtTarget();

  Camera camera_;
  bool active_ = false;
  std::optional<glm::vec3> lookAtTarget_;
};

} // namespace DL

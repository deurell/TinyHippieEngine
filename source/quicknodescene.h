#pragma once
#include "camera.h"
#include "scenenode.h"
#include <memory>

class PlaneNode;

class QuickNodeScene : public DL::SceneNode {
public:
  explicit QuickNodeScene(std::string_view glslVersionString);
  ~QuickNodeScene() override = default;
  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void bounce(float delta);
  [[nodiscard]] std::unique_ptr<PlaneNode> createPlane(DL::Camera *camera);
  void sortChildrenOnZ();

  std::string glslVersionString_;
  std::unique_ptr<DL::Camera> camera_ = nullptr;

  static constexpr uint number_of_planes = 32;
};

#pragma once

#include "lighting.h"
#include "scenenode.h"
#include <optional>

class LightNode : public DL::SceneNode {
public:
  enum class Kind { Directional };

  explicit LightNode(DL::SceneNode *parentNode = nullptr);
  ~LightNode() override = default;

  void update(const DL::FrameContext &ctx) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "LightNode";
  }

  void setKind(Kind kind) { kind_ = kind; }
  [[nodiscard]] Kind kind() const { return kind_; }
  void setActive(bool active) { active_ = active; }
  [[nodiscard]] bool active() const { return active_; }
  void setColor(glm::vec3 color) { color_ = color; }
  [[nodiscard]] glm::vec3 color() const { return color_; }
  void setIntensity(float intensity) { intensity_ = intensity; }
  [[nodiscard]] float intensity() const { return intensity_; }
  void setAmbientStrength(float ambientStrength) {
    ambientStrength_ = ambientStrength;
  }
  [[nodiscard]] float ambientStrength() const { return ambientStrength_; }
  void setDirection(glm::vec3 direction);
  [[nodiscard]] glm::vec3 direction() { return worldDirection(); }

private:
  glm::vec3 worldDirection();

  Kind kind_ = Kind::Directional;
  bool active_ = true;
  glm::vec3 color_{1.0f, 0.96f, 0.9f};
  float intensity_ = 1.0f;
  float ambientStrength_ = 0.42f;
  std::optional<glm::vec3> explicitDirection_;
};

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
  void setActive(bool active) { active_ = active; }
  void setColor(glm::vec3 color) { color_ = color; }
  void setIntensity(float intensity) { intensity_ = intensity; }
  void setAmbientStrength(float ambientStrength) {
    ambientStrength_ = ambientStrength;
  }
  void setDirection(glm::vec3 direction);

private:
  glm::vec3 worldDirection();

  Kind kind_ = Kind::Directional;
  bool active_ = true;
  glm::vec3 color_{1.0f, 0.96f, 0.9f};
  float intensity_ = 1.0f;
  float ambientStrength_ = 0.42f;
  std::optional<glm::vec3> explicitDirection_;
};

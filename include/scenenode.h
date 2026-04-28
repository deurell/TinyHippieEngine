#pragma once
#include "iscene.h"
#include "visualizerbase.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DL {

class SceneNode : public IScene {
public:
  std::vector<std::unique_ptr<SceneNode>> children;

  explicit SceneNode(SceneNode *parentNode = nullptr);
  ~SceneNode() override = default;

  // IScene methods
  void init() override;
  void update(const FrameContext &ctx) override;
  void fixedUpdate(const FrameContext &ctx) override;
  void render(const FrameContext &ctx) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "SceneNode";
  }

  void updateTransforms(const glm::mat4 &parentWorldTransform);

  void setParent(SceneNode *parent);
  void markDirty();
  void addChild(std::unique_ptr<SceneNode> child);

  void setLocalPosition(const glm::vec3 &position);
  glm::vec3 getLocalPosition() const;
  void setDebugLocalPosition(const glm::vec3 &position);

  void setLocalRotation(const glm::quat &rotation);
  glm::quat getLocalRotation() const;
  void setDebugLocalRotation(const glm::quat &rotation);

  void setLocalScale(const glm::vec3 &scale);
  glm::vec3 getLocalScale() const;
  void setDebugLocalScale(const glm::vec3 &scale);
  void setDebugName(std::string name);
  std::string_view getDebugName() const;
  bool hasParent() const { return parent != nullptr; }
  void setDebugTransformOverrideEnabled(bool enabled);
  bool isDebugTransformOverrideEnabled() const {
    return debugTransformOverrideEnabled_;
  }

  void addRenderComponent(std::unique_ptr<VisualizerBase> component);
  [[nodiscard]] std::size_t renderComponentCount() const {
    return renderComponents_.size();
  }
  [[nodiscard]] const std::vector<std::unique_ptr<VisualizerBase>> &
  renderComponents() const {
    return renderComponents_;
  }

  glm::mat4 getWorldTransform();
  glm::vec3 getWorldPosition();
  glm::quat getWorldRotation();
  glm::vec3 getWorldScale();

protected:
  SceneNode *parent;
  glm::mat4 localTransform = glm::mat4(1.0f);
  glm::mat4 worldTransform = glm::mat4(1.0f);
  bool dirty = true;
  glm::vec3 localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  glm::vec3 localScale = glm::vec3(1.0f, 1.0f, 1.0f);
  std::string debugName_ = "SceneNode";
  bool debugTransformOverrideEnabled_ = false;
  std::vector<std::unique_ptr<VisualizerBase>> renderComponents_;

private:
  void applyLocalPosition(const glm::vec3 &position);
  void applyLocalRotation(const glm::quat &rotation);
  void applyLocalScale(const glm::vec3 &scale);
  glm::mat4 extractPositionMatrix() const;
  glm::mat4 extractScaleMatrix() const;
};

} // namespace DL

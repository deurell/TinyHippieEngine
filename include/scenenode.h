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
#include <utility>
#include <vector>

namespace DL {

class SceneNode : public IScene {
public:
  std::vector<std::unique_ptr<SceneNode>> children;
  std::vector<std::unique_ptr<VisualizerBase>> visualizers;

  SceneNode(SceneNode *parentNode = nullptr)
      : dirty(true), parent(parentNode) {}
  ~SceneNode() override = default;

  // IScene methods
  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

  void updateTransforms(const glm::mat4 &parentWorldTransform);

  void setParent(SceneNode *parent);
  void markDirty();
  void addChild(std::unique_ptr<SceneNode> child);

  void setLocalPosition(const glm::vec3 &position);
  glm::vec3 getLocalPosition() const;

  void setLocalRotation(const glm::quat &rotation);
  glm::quat getLocalRotation() const;

  void setLocalScale(const glm::vec3 &scale);
  glm::vec3 getLocalScale() const;

  glm::mat4 getWorldTransform();
  glm::vec3 getWorldPosition();
  glm::quat getWorldRotation();
  glm::vec3 getWorldScale();

protected:
  SceneNode *parent;

private:
  glm::mat4 localTransform = glm::mat4(1.0f);
  glm::mat4 worldTransform;
  bool dirty = true;

  glm::vec3 localPosition;
  glm::quat localRotation;
  glm::vec3 localScale = glm::vec3(1.0f, 1.0f, 1.0f);

  glm::mat4 extractPositionMatrix() const;
  glm::mat4 extractScaleMatrix() const;
};

} // namespace DL

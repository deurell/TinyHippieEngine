#include "lightnode.h"

#include <glm/geometric.hpp>

LightNode::LightNode(DL::SceneNode *parentNode) : SceneNode(parentNode) {
  setDebugName("LightNode");
}

void LightNode::setDirection(glm::vec3 direction) {
  if (glm::length(direction) > 0.0001f) {
    explicitDirection_ = glm::normalize(direction);
  }
}

void LightNode::update(const DL::FrameContext &ctx) {
  if (active_ && kind_ == Kind::Directional && ctx.lighting != nullptr) {
    ctx.lighting->directionalEnabled = true;
    ctx.lighting->direction = worldDirection();
    ctx.lighting->color = color_;
    ctx.lighting->intensity = intensity_;
    ctx.lighting->ambientStrength = ambientStrength_;
  }
  SceneNode::update(ctx);
}

glm::vec3 LightNode::worldDirection() {
  if (explicitDirection_.has_value()) {
    return *explicitDirection_;
  }
  const glm::vec3 direction = getWorldRotation() * glm::vec3(0.0f, -1.0f, 0.0f);
  if (glm::length(direction) < 0.0001f) {
    return {0.35f, 1.0f, 0.25f};
  }
  return glm::normalize(direction);
}

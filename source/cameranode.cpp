#include "cameranode.h"

#include <glm/gtc/quaternion.hpp>

namespace DL {

CameraNode::CameraNode(SceneNode *parentNode) : SceneNode(parentNode) {}

void CameraNode::init() {
  applyLookAtTarget();
  syncCameraFromTransform();
  SceneNode::init();
}

void CameraNode::update(const FrameContext &ctx) {
  if (parentNode() != nullptr) {
    updateTransforms(parentNode()->getWorldTransform());
  } else {
    updateTransforms(glm::mat4(1.0f));
  }
  syncCameraFromTransform();

  for (auto &child : children) {
    child->update(ctx);
  }
}

void CameraNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  camera_.mScreenSize = size;
}

void CameraNode::setFov(float fov) { camera_.mFov = fov; }

void CameraNode::lookAtWorld(glm::vec3 target) {
  Camera lookAtCamera(getLocalPosition());
  lookAtCamera.lookAt(target);
  setLocalRotation(glm::inverse(lookAtCamera.mOrientation));
  syncCameraFromTransform();
}

void CameraNode::translateLocal(glm::vec3 offset) {
  syncCameraFromTransform();
  camera_.translate(offset);
  setLocalPosition(camera_.getPosition());
  syncCameraFromTransform();
}

void CameraNode::applyLookAtTarget() {
  if (!lookAtTarget_.has_value()) {
    return;
  }

  lookAtWorld(*lookAtTarget_);
}

void CameraNode::syncCameraFromTransform() {
  if (parentNode() != nullptr) {
    updateTransforms(parentNode()->getWorldTransform());
  } else {
    updateTransforms(glm::mat4(1.0f));
  }
  camera_.mPosition = getWorldPosition();
  camera_.mOrientation = glm::inverse(getWorldRotation());
}

} // namespace DL

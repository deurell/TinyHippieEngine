#include "scenenode.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace DL {

void SceneNode::updateTransforms(const glm::mat4& parentWorldTransform) {
  if (dirty) {
    glm::mat4 newTransform = localTransform;
    worldTransform = parentWorldTransform * newTransform;
    dirty = false;
  }

  for (auto& child : children) {
    child->updateTransforms(worldTransform);
  }
}

void SceneNode::render(float delta) {

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("Node Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif

  updateTransforms(glm::mat4(1.0f));

  for (auto& component : components) {
    component->render(worldTransform, delta);
  }

  for (auto& child : children) {
    child->render(delta);
  }
}

void SceneNode::setLocalPosition(const glm::vec3& position) {
  localTransform[3] = glm::vec4(position, 1.0f);
  dirty = true;
}

glm::vec3 SceneNode::getLocalPosition() const {
  return glm::vec3(localTransform[3]);
}

void SceneNode::setLocalRotation(const glm::vec3& rotation) {
  glm::mat4 rotationMatrix(1.0f);
  rotationMatrix = glm::rotate(rotationMatrix, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
  rotationMatrix = glm::rotate(rotationMatrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
  rotationMatrix = glm::rotate(rotationMatrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
  localTransform = localTransform * rotationMatrix;
  dirty = true;
}

glm::vec3 SceneNode::getLocalRotation() const {
  glm::vec3 rotation;
  rotation.x = atan2(localTransform[2][1], localTransform[2][2]);
  rotation.y = atan2(-localTransform[2][0], sqrt(localTransform[2][1] * localTransform[2][1] + localTransform[2][2] * localTransform[2][2]));
  rotation.z = atan2(localTransform[1][0], localTransform[0][0]);
  return rotation;
}

void SceneNode::setLocalScale(const glm::vec3& scale) {
  glm::mat4 scaleMatrix(1.0f);
  scaleMatrix[0][0] = scale.x;
  scaleMatrix[1][1] = scale.y;
  scaleMatrix[2][2] = scale.z;
  localTransform = localTransform * scaleMatrix;
  dirty = true;
}

glm::vec3 SceneNode::getLocalScale() const {
  return glm::vec3(glm::length(localTransform[0]), glm::length(localTransform[1]), glm::length(localTransform[2]));
}
void SceneNode::init() {}

void SceneNode::onClick(double x, double y) {}

void SceneNode::onKey(int key) {}

void SceneNode::onScreenSizeChanged(glm::vec2 size) {

}
SceneNode::SceneNode() {}

} // namespace DL

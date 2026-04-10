#include "scenelifecycle.h"

namespace DL {

std::unique_ptr<IScene> prepareScene(std::unique_ptr<IScene> scene,
                                     glm::vec2 windowSize,
                                     glm::vec2 framebufferSize) {
  if (!scene) {
    return nullptr;
  }

  scene->init();
  scene->onScreenSizeChanged(windowSize);
  scene->onFramebufferSizeChanged(framebufferSize);
  return scene;
}

std::unique_ptr<IScene> replacePreparedScene(
    std::unique_ptr<IScene> currentScene, std::unique_ptr<IScene> nextScene,
    glm::vec2 windowSize, glm::vec2 framebufferSize) {
  auto preparedScene =
      prepareScene(std::move(nextScene), windowSize, framebufferSize);
  if (!preparedScene) {
    return currentScene;
  }
  return preparedScene;
}

} // namespace DL

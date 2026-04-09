#pragma once

#include "iscene.h"
#include <glm/glm.hpp>
#include <memory>

namespace DL {

auto prepareScene(std::unique_ptr<IScene> scene, glm::vec2 windowSize,
                  glm::vec2 framebufferSize) -> std::unique_ptr<IScene>;
auto replacePreparedScene(std::unique_ptr<IScene> currentScene,
                          std::unique_ptr<IScene> nextScene,
                          glm::vec2 windowSize,
                          glm::vec2 framebufferSize)
    -> std::unique_ptr<IScene>;

} // namespace DL

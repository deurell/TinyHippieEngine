//
// Created by Mikael Deurell on 2023-08-15.
//

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace DL {
class IComponent {
public:
  virtual void render(const glm::mat4 &worldTransform, float delta) = 0;
  virtual ~IComponent() = default;
};
} // namespace DL

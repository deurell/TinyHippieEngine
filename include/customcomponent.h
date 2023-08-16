#pragma once
#include "Icomponent.h"
#include <glm/glm.hpp>
#include <iostream>
#include <string>

namespace DL {

class CustomComponent : public IComponent {
public:
  explicit CustomComponent(std::string name) : name(std::move(name)) {}

  void render(const glm::mat4 &worldTransform, float delta) override {
    std::cout << "Rendering component: " << name << std::endl;
  }

private:
  std::string name;
};
}; // namespace DL
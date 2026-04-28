#pragma once
#include <array>
#include <glm/glm.hpp>
#include <string>
#include <string_view>

namespace DL {
enum class Key {
  W,
  A,
  S,
  D,
  Count,
};

enum class Action {
  MoveForward,
  MoveBackward,
  MoveLeft,
  MoveRight,
  Count,
};

enum class MouseButton {
  Left,
  Right,
  Middle,
  Count,
};

struct InputState {
  std::array<bool, static_cast<std::size_t>(Key::Count)> keysDown{};
  std::array<bool, static_cast<std::size_t>(Action::Count)> actionsDown{};
  std::array<bool, static_cast<std::size_t>(MouseButton::Count)> mouseButtonsDown{};
  glm::vec2 mouseDelta{0.0f};

  [[nodiscard]] bool isKeyDown(Key key) const {
    return keysDown[static_cast<std::size_t>(key)];
  }

  [[nodiscard]] bool isActionDown(Action action) const {
    return actionsDown[static_cast<std::size_t>(action)];
  }

  [[nodiscard]] bool isMouseButtonDown(MouseButton button) const {
    return mouseButtonsDown[static_cast<std::size_t>(button)];
  }
};

class ActionMap {
public:
  ActionMap() { bindings_.fill(Key::Count); }

  void bind(Action action, Key key) {
    bindings_[static_cast<std::size_t>(action)] = key;
  }

  [[nodiscard]] Key binding(Action action) const {
    return bindings_[static_cast<std::size_t>(action)];
  }

  void apply(const InputState &sourceKeys, InputState &target) const {
    target.actionsDown.fill(false);
    for (std::size_t index = 0; index < bindings_.size(); ++index) {
      const Key key = bindings_[index];
      if (key == Key::Count) {
        continue;
      }
      target.actionsDown[index] =
          sourceKeys.keysDown[static_cast<std::size_t>(key)];
    }
  }

private:
  std::array<Key, static_cast<std::size_t>(Action::Count)> bindings_{};
};

struct FrameContext {
  float delta_time = 0.0f;
  double total_time = 0.0;
  InputState input;
};

class IScene {
public:
  virtual ~IScene() = default;

  virtual void init() = 0;
  virtual void update(const FrameContext &ctx) = 0;
  virtual void fixedUpdate(const FrameContext &ctx) {}
  virtual void render(const FrameContext &ctx) = 0;
  virtual void onClick(double x, double y) = 0;
  virtual void onKey(int key) = 0;
  virtual void onScreenSizeChanged(glm::vec2 size) = 0;
  virtual void onFramebufferSizeChanged(glm::vec2 size) {
    onScreenSizeChanged(size);
  }
  [[nodiscard]] virtual std::string_view debugTypeName() const {
    return "IScene";
  }
};
} // namespace DL

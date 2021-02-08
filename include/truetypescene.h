#pragma once

#include "IScene.h"
#include "camera.h"
#include "shader.h"
#include "stb_truetype.h"
#include "textsprite.h"
#include "texture.h"
#include <memory>

class TrueTypeScene : public DL::IScene {

  enum class SceneState { INTRO, RUNNING, OUTRO };

public:
  explicit TrueTypeScene(std::string_view glslVersion);
  ~TrueTypeScene() override = default;

  void init() override;
  void render(float delta) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void renderScroll(float delta);
  void renderStatus(float delta);
  void calculateStatus(float delta);

  static inline glm::vec3 lerp(glm::vec3 x, glm::vec3 y, float t);

  static inline float normalizedBezier3(float b, float c, float t) {
    float s = 1.f - t;
    float t2 = t * t;
    float s2 = s * s;
    float t3 = t2 * t;
    return (3.f * b * s2 * t) + (3.f * c * s * t2) + t3;
  }

  std::unique_ptr<DL::Shader> mLabelShader;
  std::unique_ptr<DL::Camera> mLabelCamera;
  std::unique_ptr<DL::TextSprite> mTextSprite;

  std::unique_ptr<DL::Shader> mStatusShader;
  std::unique_ptr<DL::TextSprite> mStatusSprite;

  std::string mGlslVersionString;
  glm::vec2 mScreenSize = {0.0, 0.0};
  float mScrollOffset = 0;
  const float scroll_wrap = 52;
  float mDelta = 0;

  float mStateStartTime = 0;
  glm::vec3 mStatusOffset = {0.0, 0.0, 0.0};
  static constexpr float mIntroTime = 3.0;
  SceneState mState = SceneState::INTRO;
};

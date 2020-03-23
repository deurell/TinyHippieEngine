#pragma once

#include "IScene.h"
#include "camera.h"
#include "model.h"
#include "shader.h"
#include <memory>

class DemoScene : public IScene {
public:
  DemoScene(std::string glslVersion,
            basist::etc1_global_selector_codebook *codeBook);
  ~DemoScene() = default;

  virtual void init() override;
  virtual void render(float delta) override;
  virtual void onKey(int key, float delta) override;
  virtual void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::unique_ptr<Shader> mLightingShader;
  std::unique_ptr<Shader> mLampShader;
  std::unique_ptr<Camera> mCamera;
  std::unique_ptr<Model> mModel;
  unsigned int mLightVAO;
  unsigned int mCubeVAO;
  glm::vec3 mLightPos = {1.2f, 1.0f, 2.0f};
  static constexpr int point_light_count = 2;
  glm::vec3 mPointLightPositions[point_light_count] = {
      glm::vec3(0.7f, 0.2f, 2.0f), glm::vec3(2.3f, -3.3f, -4.0f)};
  float mXDeg = 0.0f;
  float mYDeg = 0.0f;
  glm::vec3 mModelTranslate = {0.0f, 0.0f, 0.0f};
  glm::vec3 mObjectColor = {0.3, 0.8, 0.3};
  std::string mGlslVersionString;
  basist::etc1_global_selector_codebook *mCodeBook;
  glm::vec2 mScreenSize;
};

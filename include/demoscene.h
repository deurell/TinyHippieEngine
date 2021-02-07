#pragma once

#include "IScene.h"
#include "camera.h"
#include "model.h"
#include "shader.h"
#include <memory>

class DemoScene : public DL::IScene {
public:
  DemoScene(std::string glslVersion,
            basist::etc1_global_selector_codebook *codeBook);
  ~DemoScene() override = default;

  void init() override;
  void render(float delta) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::unique_ptr<DL::Shader> mLightingShader;
  std::unique_ptr<DL::Shader> mLampShader;
  std::unique_ptr<DL::Camera> mCamera;
  std::unique_ptr<DL::Model> mModel;
  unsigned int mLightVAO = 0;
  unsigned int mCubeVAO = 0;
  glm::vec3 mLightPos = {1.2f, 1.0f, 2.0f};
  static constexpr int point_light_count = 2;
  glm::vec3 mPointLightPositions[point_light_count] = {
      glm::vec3(0.7f, 0.2f, 2.0f), glm::vec3(2.3f, -3.3f, -4.0f)};
  float mXDeg = 0.0f;
  float mYDeg = 0.0f;
  glm::vec3 mModelTranslate = {0.0f, 0.0f, 0.0f};
  std::string mGlslVersionString;
  basist::etc1_global_selector_codebook *mCodeBook;
  glm::vec2 mScreenSize ={0.0,0.0};
  float mDelta = 0;
};

#pragma once

#include "camera.h"
#include "iscene.h"
#include "model.h"
#include "shader.h"
#include <memory>

class DemoScene : public DL::IScene {
public:
  DemoScene(std::string_view glslVersion,
            basist::etc1_global_selector_codebook *codeBook);
  ~DemoScene() override = default;

  void init() override;
  void render(float delta) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::unique_ptr<DL::Shader> lightingShader_;
  std::unique_ptr<DL::Shader> lampShader_;
  std::unique_ptr<DL::Camera> camera_;
  std::unique_ptr<DL::Model> model_;
  unsigned int lightVAO_ = 0;
  unsigned int cubeVAO_ = 0;
  glm::vec3 lightPos_ = {1.2f, 1.0f, 2.0f};
  static constexpr int point_light_count = 2;
  glm::vec3 pointLightPositions_[point_light_count] = {
      glm::vec3(0.7f, 0.2f, 2.0f), glm::vec3(2.3f, -3.3f, -4.0f)};
  float xDeg_ = 0.0f;
  float yDeg_ = 0.0f;
  glm::vec3 modelTranslate_ = {0.0f, 0.0f, 0.0f};
  std::string glslVersionString_;
  basist::etc1_global_selector_codebook *codeBook_;
  glm::vec2 screenSize_ = {0.0, 0.0};
  float delta_ = 0;
};

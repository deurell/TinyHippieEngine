#pragma once

#include "audioplayer.h"
#include "camera.h"
#include "iscene.h"
#include "model.h"
#include "shader.h"
#include "texture.h"
#include <memory>

class C64Scene : public DL::IScene {
public:
  C64Scene(std::string_view glslVersion,
           basist::etc1_global_selector_codebook *codeBook);

  C64Scene(const C64Scene &rhs) = delete;

  ~C64Scene() override = default;

  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  const char* audio_unlock = "unlock";
  std::unique_ptr<DL::Shader> shader_;
  std::unique_ptr<DL::Texture> texture_;
  basist::etc1_global_selector_codebook *codeBook_;

  unsigned int VAO_ = 0;
  unsigned int VBO_ = 0;
  unsigned int EBO_ = 0;
  std::string glslVersionString_;
  glm::vec2 screenSize_{};
  float delta_ = 0;
  std::unique_ptr<AudioPlayer> audioPlayer_;
};

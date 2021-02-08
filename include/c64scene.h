#pragma once

#include "IScene.h"
#include "camera.h"
#include "model.h"
#include "shader.h"
#include "texture.h"
#include <memory>

class C64Scene : public DL::IScene {
public:
  C64Scene(std::string_view glslVersion,
           basist::etc1_global_selector_codebook &codeBook);
  
  C64Scene(const C64Scene& rhs) = delete;
  
  ~C64Scene() override = default;

  void init() override;
  void render(float delta) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::unique_ptr<DL::Shader> mShader;
  std::unique_ptr<DL::Texture> mTexture;
  basist::etc1_global_selector_codebook &mCodeBook;

  unsigned int mVAO = 0;
  unsigned int mVBO = 0;
  unsigned int mEBO = 0;
  std::string_view mGlslVersionString;
  glm::vec2 mScreenSize{};
  float mDelta = 0;
};

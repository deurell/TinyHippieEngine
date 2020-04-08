#pragma once

#include "IScene.h"
#include "camera.h"
#include "model.h"
#include "shader.h"
#include "texture.h"
#include <memory>

class C64Scene : public DL::IScene {
public:
  C64Scene(std::string glslVersion,
           basist::etc1_global_selector_codebook &codeBook);
  ~C64Scene() = default;

  virtual void init() override;
  virtual void render(float delta) override;
  virtual void onKey(int key) override;
  virtual void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::unique_ptr<DL::Shader> mShader;
  std::unique_ptr<DL::Texture> mTexture;
  basist::etc1_global_selector_codebook &mCodeBook;

  unsigned int mVAO, mVBO, mEBO;
  std::string mGlslVersionString;
  glm::vec2 mScreenSize;
  float mDelta;
};
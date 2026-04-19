#pragma once

#include "audiosystem.h"
#include "camera.h"
#include "iscene.h"
#include "model.h"
#include "renderdevice.h"
#include "shader.h"
#include "texture.h"
#include <memory>

class C64Scene : public DL::IScene {
public:
  C64Scene(std::string_view glslVersion,
           basist::etc1_global_selector_codebook *codeBook,
           DL::IRenderDevice *renderDevice,
           DL::AudioSystem *audioSystem);

  C64Scene(const C64Scene &rhs) = delete;

  ~C64Scene() override;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  static constexpr const char *kAudioUnlock = "unlock";
  basist::etc1_global_selector_codebook *codeBook_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::AudioSystem *audioSystem_ = nullptr;
  DL::MeshHandle mesh_;
  DL::TextureHandle texture_;
  DL::PipelineHandle pipeline_;
  std::string glslVersionString_;
  glm::vec2 screenSize_{};
  float delta_ = 0;
};

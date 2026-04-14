#include "c64scene.h"
#include "debugui.h"
#ifdef USE_IMGUI
#include "imgui.h"
#endif
#include <iostream>

C64Scene::C64Scene(std::string_view glslVersion,
                   basist::etc1_global_selector_codebook *codeBook,
                   DL::IRenderDevice *renderDevice,
                   DL::AudioSystem *audioSystem)
    : glslVersionString_(glslVersion), codeBook_(codeBook),
      renderDevice_(renderDevice), audioSystem_(audioSystem) {}

C64Scene::~C64Scene() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid()) {
      renderDevice_->destroy(mesh_);
    }
    if (texture_.valid()) {
      renderDevice_->destroy(texture_);
    }
    if (pipeline_.valid()) {
      renderDevice_->destroy(pipeline_);
    }
  }
}

void C64Scene::init() {
  if (renderDevice_ == nullptr || codeBook_ == nullptr) {
    return;
  }
  if (audioSystem_ != nullptr) {
    audioSystem_->loadClip(kAudioUnlock, "Resources/unlock.wav");
    audioSystem_->playOneShot(kAudioUnlock);
  }
  pipeline_ = renderDevice_->createPipeline("Shaders/c64.vert",
                                            "Shaders/c64.frag",
                                            glslVersionString_);
  mesh_ = renderDevice_->createTexturedQuad();
  texture_ =
      renderDevice_->createBasisTexture("Resources/sup.basis", *codeBook_);
}

void C64Scene::update(const DL::FrameContext & /*ctx*/) {}

void C64Scene::render(const DL::FrameContext &ctx) {
  delta_ = ctx.delta_time;
  glClearColor(0.52f, 0.81f, .92f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glClearColor(0.3, 0.3, 0.3, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  glm::mat4 transform = glm::mat4(1.0f);
  transform = glm::rotate(transform, glm::radians<float>(0),
                          glm::vec3(0.0f, 0.0f, 1.0f));
  if (renderDevice_ != nullptr && mesh_.valid() && pipeline_.valid() &&
      texture_.valid()) {
    DL::DrawCommand command;
    command.mesh = mesh_;
    command.pipeline = pipeline_;
    command.texture = texture_;
    command.uniforms.push_back(
        DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
    command.uniforms.push_back(
        DL::UniformValue::makeMat4("transform", transform));
    renderDevice_->draw(command);
  }

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("tiny hippie engine");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif
}

void C64Scene::onClick(double x, double y) {}

void C64Scene::onKey(int key) {}

void C64Scene::onScreenSizeChanged(glm::vec2 size) { screenSize_ = size; }

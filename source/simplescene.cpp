//
// Created by Mikael Deurell on 2021-12-15.
//

#include "simplescene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

SimpleScene::SimpleScene(std::string_view glslVersionString,
                         DL::IRenderDevice *renderDevice)
    : renderDevice_(renderDevice), mGlslVersionString(glslVersionString) {}

SimpleScene::~SimpleScene() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid()) {
      renderDevice_->destroy(mesh_);
    }
    if (pipeline_.valid()) {
      renderDevice_->destroy(pipeline_);
    }
  }
}

void SimpleScene::init() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});
  if (renderDevice_ == nullptr) {
    return;
  }
  pipeline_ = renderDevice_->createPipeline("Shaders/simple.vert",
                                            "Shaders/simple.frag",
                                            mGlslVersionString);
  mesh_ = renderDevice_->createTexturedQuad();
}

void SimpleScene::update(const DL::FrameContext & /*ctx*/) {}

void SimpleScene::render(const DL::FrameContext &ctx) {
  glDisable(GL_DEPTH_TEST);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  if (renderDevice_ != nullptr && mesh_.valid() && pipeline_.valid()) {
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = mCamera->getViewMatrix();
    glm::mat4 projection = mCamera->getPerspectiveTransform();

    DL::DrawCommand command;
    command.mesh = mesh_;
    command.pipeline = pipeline_;
    command.uniforms.push_back(
        DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
    command.uniforms.push_back(
        DL::UniformValue::makeVec4("baseColor", {0.2f, 0.7f, 1.0f, 1.0f}));
    command.uniforms.push_back(DL::UniformValue::makeMat4("model", model));
    command.uniforms.push_back(DL::UniformValue::makeMat4("view", view));
    command.uniforms.push_back(
        DL::UniformValue::makeMat4("projection", projection));
    renderDevice_->draw(command);
  }

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("simple scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif
}

void SimpleScene::onClick(double x, double y) {
  std::cout << "Mouse click @ x:" << x << " y:" << y << std::endl;
}

void SimpleScene::onKey(int key) { std::cout << key << std::endl; }

void SimpleScene::onScreenSizeChanged(glm::vec2 size) {
  mScreenSize = size;
  mCamera->mScreenSize = size;
}

//
// Created by Mikael Deurell on 2021-12-15.
//

#include "simplescene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

SimpleScene::SimpleScene(std::string_view glslVersionString)
    : mGlslVersionString(glslVersionString) {}

void SimpleScene::init() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});
  auto shader = std::make_unique<DL::Shader>(
      "Shaders/simple.vert", "Shaders/simple.frag", mGlslVersionString);

  mPlane = std::make_unique<DL::Plane>(std::move(shader), *mCamera);
  mPlane->mPosition = {0, 0, 0};
  mPlane->mScale = {6, 6, 1};
}

void SimpleScene::render(float delta) {
  glClearColor(0.5, 0.5, 0.5, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  mPlane->render(delta);

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("simple scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif
}

void SimpleScene::onKey(int key) {}

void SimpleScene::onScreenSizeChanged(glm::vec2 size) {
  mScreenSize = size;
  mCamera->mScreenSize = size;
}

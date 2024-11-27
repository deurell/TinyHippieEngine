#include "quicknodescene.h"
#include "planenode.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <memory>

QuickNodeScene::QuickNodeScene(std::string_view glslVersionString)
    : glslVersionString_(glslVersionString) {}

void QuickNodeScene::init() {
  SceneNode::init();
  auto plane = std::make_unique<PlaneNode>(glslVersionString_);
  plane->planeType = PlaneNode::PlaneType::Simple;
  plane->init();

  plane_ = plane.get();
  addChild(std::move(plane));
}

void QuickNodeScene::update(float delta) { SceneNode::update(delta); }

void QuickNodeScene::render(float delta) {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  SceneNode::render(delta);

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("Quick Node Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif

}

void QuickNodeScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

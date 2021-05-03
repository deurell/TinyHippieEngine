#include "introscene.h"

IntroScene::IntroScene(std::string_view glslVersionString) : mGlslVersionString(glslVersionString) {
}

void IntroScene::init() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0,0,0});
  mLogoShader = std::make_unique<DL::Shader>("Shaders/intro_logo.vert", "Shaders/intro_logo.frag", mGlslVersionString);
  std::string logoText = "SECTOR 90";
  mLogoSprite = std::make_unique<DL::TextSprite>("Resources/C64_Pro-STYLE.ttf", logoText);
}

void IntroScene::render(float delta) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("tiny hippie engine");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif

}

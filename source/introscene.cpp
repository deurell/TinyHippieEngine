#include "introscene.h"

IntroScene::IntroScene(std::string_view glslVersionString) : mGlslVersionString(glslVersionString) {
}

void IntroScene::init() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0,0,0});
  mLogoShader = std::make_unique<DL::Shader>("Shaders/intro_logo.vert", "Shaders/intro_logo.frag", mGlslVersionString);
  std::string logoText = "SECTOR 90";
  mLogoSprite = std::make_unique<DL::TextSprite>("Resources/C64_Pro-STYLE.ttf", logoText);

  // raster bars
  mPlane = std::make_unique<DL::Plane>("Shaders/rasterbars.vert", "Shaders/rasterbars.frag", mGlslVersionString, *mCamera);
  mPlane->mPosition = {0,0.0,0};
  mPlane->mScale = {40, 3, 1};
}

void IntroScene::render(float delta) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  renderLogo(delta);
  mPlane->render(delta);


#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("tiny intro");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif

}

void IntroScene::renderLogo(float delta) {
  mLogoShader->use();
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::scale(model, glm::vec3(0.06, 0.06, 1.0));
  model = glm::translate(model, glm::vec3(-158.0, -16.0, 0.0));
  mLogoShader->setMat4f("model", model);
  glm::mat4 view = mCamera->getViewMatrix();
  mLogoShader->setMat4f("view", view);
  glm::mat4 projectionMatrix = mCamera->getPerspectiveTransform(
      45.0, mScreenSize.x / mScreenSize.y);
  mLogoShader->setMat4f("projection", projectionMatrix);

  mLogoShader->setFloat("iTime", static_cast<float>(glfwGetTime()));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mLogoSprite->mFontTexture);
  mLogoShader->setInt("texture1", 0);

  mLogoSprite->render(delta);
}

void IntroScene::onScreenSizeChanged(glm::vec2 size) {
  mScreenSize = size;
  mCamera->mScreenSize = mScreenSize;
}

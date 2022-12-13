#include "introscene.h"

IntroScene::IntroScene(std::string_view glslVersionString)
    : mGlslVersionString(glslVersionString) {}

void IntroScene::init() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});
  mLogoShader = std::make_unique<DL::Shader>(
      "Shaders/intro_logo.vert", "Shaders/intro_logo.frag", mGlslVersionString);
  std::string logoText = "S\xC5""D\xc4""RJA!";
  mLogoSprite =
      std::make_unique<DL::TextSprite>("Resources/c64.ttf", logoText);

  // raster bars
  std::unique_ptr<DL::Shader> shader = std::make_unique<DL::Shader>(
      "Shaders/rasterbars.vert", "Shaders/rasterbars.frag", mGlslVersionString);

  mPlane = std::make_unique<DL::Plane>(
      std::move(shader), *mCamera, [this](DL::Shader &shader) {
        shader.setFloat("sineOffset", this->mOffset);
        shader.setFloat("tweak", mTweak);
        shader.setFloat("bars", mBars);
        shader.setVec3f("baseCol", mBaseCol);
      });
  mPlane->mPosition = {0, 1.4, 0};
  mPlane->mScale = {40, 3, 1};

  std::unique_ptr<DL::Shader> shader2 = std::make_unique<DL::Shader>(
      "Shaders/rasterbars.vert", "Shaders/rasterbars.frag", mGlslVersionString);

  mPlane2 = std::make_unique<DL::Plane>(
      std::move(shader2), *mCamera, [this](DL::Shader &shader) {
        shader.setFloat("sineOffset", glm::pi<float>() + this->mOffset);
        shader.setFloat("tweak", mTweak);
        shader.setFloat("bars", mBars);
        shader.setVec3f("baseCol", mBaseCol);
      });

  mPlane2->mPosition = {0, -1.4, 0};
  mPlane2->mScale = {40, 3, 1};
}

void IntroScene::render(float delta) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  renderLogo(delta);
  mPlane->render(delta);
  mPlane2->render(delta);

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("tiny intro");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::SliderFloat("offset", &mOffset, 0, 5);
  ImGui::SliderFloat("tweak", &mTweak, -50, 50);
  ImGui::SliderFloat("logoOffset", &mLogoOffset, -200, 200);
  ImGui::ColorPicker3("color2", (float*)&mBaseCol);
  ImGui::End();
#endif
}

void IntroScene::renderLogo(float delta) {
  mLogoShader->use();
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::scale(model, glm::vec3(0.06, 0.06, 1.0));
  model = glm::translate(model, glm::vec3(mLogoOffset, -16.0, 0.0));
  mLogoShader->setMat4f("model", model);
  glm::mat4 view = mCamera->getViewMatrix();
  mLogoShader->setMat4f("view", view);
  glm::mat4 projectionMatrix =
      mCamera->getPerspectiveTransform(45.0, mScreenSize.x / mScreenSize.y);
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
void IntroScene::onClick(double x, double y) {
  std::cout << "clicked: " << x << ", " << y << std::endl;
}
void IntroScene::onKey(int key) {
  std::cout << "pressed: " << key << std::endl;
}

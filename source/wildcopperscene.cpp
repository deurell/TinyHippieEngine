#include "wildcopperscene.h"
#include "GLFW/glfw3.h"
#include <cmath>

WildCopperScene::WildCopperScene(std::string_view glslVersionString)
    : mGlslVersionString(glslVersionString) {}

void WildCopperScene::init() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 68));
  mCamera->lookAt({0, 0, 0});
  mShader = std::make_unique<DL::Shader>(
      "Shaders/wildcopper.vert", "Shaders/wildcopper.frag", mGlslVersionString);
  std::string scrollText = "    WILDCOPPER... WE LOVE "
                           "YOU!!! THE WAY I REMEMBER IT.... :) <3            ";
  mTextSprite = std::make_unique<DL::TextSprite>("Resources/C64_Pro-STYLE.ttf",
                                                 scrollText);

  for (int i = 0; i < scrollText.length(); ++i) {
    auto chr = std::make_unique<ScrollChar>(
        *mCamera, *mShader, mScreenSize, mTextSprite->mFontTexture,
        mTextSprite->getFontCharInfoPtr(), scrollText.substr(i, 1));
    chr->angle = -M_PI / 180 * 12.8 * i;
    mScrollChars.emplace_back(std::move(chr));
  }

  std::unique_ptr<DL::Shader> shader = std::make_unique<DL::Shader>(
      "Shaders/rast.vert", "Shaders/rast.frag", mGlslVersionString);

  mPlane = std::make_unique<DL::Plane>(std::move(shader), *mCamera);
  mPlane->position = {0, 0, 0};
  mPlane->scale = {50, 30, 1};
}

void WildCopperScene::update(float delta) {}

void WildCopperScene::render(float delta) {
  glClearColor(0.0, 0.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  renderScroller(delta);
  mPlane->render(delta);
#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("wild_copper");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif
}

void WildCopperScene::renderScroller(float delta) {
  for (auto &&sprite : mScrollChars) {
    if (sprite->angle <= M_PI * 2 && sprite->angle >= 0) {
      sprite->render(delta);
    }
    sprite->angle += M_PI / 180 * delta * 30;
    sprite->flipDegree = glfwGetTime();
  }
  if (mScrollChars.front()->angle > M_PI * 8) {
    wrap();
  }
}

void WildCopperScene::onScreenSizeChanged(glm::vec2 size) {
  mCamera->mScreenSize = size;
  mScreenSize = size;
  for (auto &&sprite : mScrollChars) {
    sprite->screenSize = size;
  }
}

void WildCopperScene::wrap() {
  for (int i = 0; i < mScrollChars.size(); ++i) {
    mScrollChars.at(i)->angle = -M_PI / 180 * 12.8 * i;
  }
}
void ScrollChar::render(float delta) {
  shader.use();

  glm::mat4 model = glm::mat4(1.0f);

  model = glm::scale(model, glm::vec3(0.11, 0.11, 0.3));
  glm::vec3 polarCoord{amp * cos(angle), amp * sin(angle), 20.0f};
  model = glm::translate(model, polarCoord);
  model =
      glm::rotate(model, angle - (float)M_PI_2, glm::vec3(0.0f, 0.0f, 1.0f));

  model = glm::translate(model, glm::vec3(0.0f, 16.0f, 0.0f));
  model = glm::rotate(model, flipDegree + angle, glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::translate(model, glm::vec3(0.0f, -16.0f, 0.0f));

  // model = scale * translate * rotate * glm::inverse(locTra) * rotLoc *
  // locTra;

  glm::mat4 view = camera.getViewMatrix();

  glm::mat4 projection = camera.getPerspectiveTransform();
  glm::mat4 mvp = projection * view * model;
  shader.setMat4f("mvp", mvp);

  shader.setFloat("deg", angle);
  shader.setFloat("iTime", static_cast<float>(glfwGetTime()));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, charSprite->mFontTexture);
  shader.setInt("texture1", 0);

  charSprite->render(delta);
}

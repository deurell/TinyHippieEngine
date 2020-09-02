#include "truetypescene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "texture.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>

TrueTypeScene::TrueTypeScene(std::string glslVersion)
    : mGlslVersionString(glslVersion) {}

void TrueTypeScene::renderScroll(float delta) {

  mLabelShader->use();

  glm::vec3 pivot = {260, 0, 0};
  glm::mat4 trans_to_pivot = glm::translate(glm::mat4(1.0f), -pivot);
  glm::mat4 trans_from_pivot = glm::translate(glm::mat4(1.0f), pivot);

  glm::mat4 rotate_matrix = glm::rotate(
      glm::mat4(1.0f), glm::radians<float>(-sin(glfwGetTime()) * 84),
      glm::vec3(0.0, 1.0, 0.0));

  glm::mat4 rotate = trans_from_pivot * rotate_matrix * trans_to_pivot;
  glm::mat4 scale_matrix =
      glm::scale(glm::mat4(1.0f), glm::vec3(0.06, 0.06, 0.06));
  rotate = scale_matrix * trans_to_pivot * rotate;
  mLabelShader->setMat4f("model", rotate);

  glm::mat4 view = mLabelCamera->getViewMatrix();
  mLabelShader->setMat4f("view", view);

  glm::mat4 projectionMatrix = mLabelCamera->getPerspectiveTransform(
      45.0, mScreenSize.x / mScreenSize.y);
  mLabelShader->setMat4f("projection", projectionMatrix);

  mLabelShader->setFloat("iTime", glfwGetTime());
  mLabelShader->setFloat("scrollOffset", mScrollOffset);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mTextSprite->mFontTexture);
  mLabelShader->setInt("texture1", 0);

  mTextSprite->render(delta);
}

void TrueTypeScene::calculateStatus(float delta) {
  if (mState != SceneState::INTRO) return;
  float timeSinceStart = glfwGetTime() - mStartTime;
  float t = timeSinceStart / intro_time;
  float mt = 1.0f-t;
  mStatusOffset = lerp(glm::vec3(240.0,0.0,0.0), glm::vec3(0.0,0.0,0.0), 1.0f-(mt*mt*mt*mt));
}

void TrueTypeScene::renderStatus(float delta) {
  mStatusShader->use();

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(-14.0,9.0,0.0) + mStatusOffset);
  model = glm::scale(model, glm::vec3(0.03,0.03,1.0));
  mStatusShader->setMat4f("model", model);
  glm::mat4 view = mLabelCamera->getViewMatrix();
  mStatusShader->setMat4f("view", view);
  glm::mat4 projectionMatrix = mLabelCamera->getPerspectiveTransform(
      45.0, mScreenSize.x / mScreenSize.y);
  mStatusShader->setMat4f("projection", projectionMatrix);

  mStatusShader->setFloat("iTime", glfwGetTime());
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mStatusSprite->mFontTexture);
  mStatusShader->setInt("texture1", 0);
  
  mStatusSprite->render(delta);
}

void TrueTypeScene::init() {
  mStartTime = glfwGetTime();

  mLabelCamera = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 26.0f));
  mLabelCamera->lookAt({0.0f, 0.0f, 0.0f});

  mLabelShader = std::make_unique<DL::Shader>(
      "Shaders/label.vert", "Shaders/label.frag", mGlslVersionString);

  std::string text =
      "                             "
      "Gruwl of Sector 90 welcomes you "
      "back to this "
      "new production released 35 "
      "years too late... Hope you enjoy it. Have a nice day out there and take "
      "care of each other!               ";

  mTextSprite = std::make_unique<DL::TextSprite>("Resources/C64_Pro-STYLE.ttf", text);

  mStatusShader = std::make_unique<DL::Shader>("Shaders/status.vert", "Shaders/status.frag", mGlslVersionString);
  std::string statusText = "truetype scroller";
  mStatusSprite = std::make_unique<DL::TextSprite>(mTextSprite->mFontTexture, mTextSprite->getFontCharInfoPtr(), statusText);
}

void TrueTypeScene::render(float delta) {
  if (glfwGetTime() - mStartTime >= intro_time){
    mState = SceneState::RUNNING;
  }
  mDelta = delta;
  mScrollOffset += delta;
  if (mScrollOffset > scroll_wrap) {
    mScrollOffset = 0;
  }

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  
  renderScroll(delta);
  calculateStatus(delta);
  renderStatus(delta);

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();  
  ImGui::NewFrame();
  ImGui::Begin("tiny hippie engine");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("scrollOffset: %.1f", mScrollOffset);
  ImGui::End();
#endif
}

void TrueTypeScene::onKey(int key){};

void TrueTypeScene::onScreenSizeChanged(glm::vec2 size) { mScreenSize = size; };

#include "particlescene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <numeric>

ParticleScene::ParticleScene(std::string_view glslVersionString)
    : mGlslVersionString(glslVersionString) {}

void ParticleScene::init() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});
  auto shader = std::make_unique<DL::Shader>(
      "Shaders/particle.vert", "Shaders/particle.frag", mGlslVersionString);

  mPlane = std::make_unique<DL::Plane>(std::move(shader), *mCamera);
  mPlane->mPosition = {0, 0, 0};
  mPlane->mScale = {0.5, 0.5, 0.5};

  particle = std::make_unique<DL::Particle>(mPlane->mPosition, 1.0, glm::vec3(0,-10.0f,0));
}

void ParticleScene::render(float delta) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  particle->updatePhysics(delta);
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

void ParticleScene::onClick(double x, double y) {
  std::cout << "Mouse click @ x:" << x << " y:" << y << std::endl;
}

void ParticleScene::onKey(int key) {
  std::cout << key << std::endl;
  glm::vec3 explode = {400, 800, 0};
  glm::vec3 dampen = {-200, 0, 0};
  switch (key) {
  case 49:
    mForces.emplace_back(explode);
    break;
  case 50:
    mForces.emplace_back(dampen);
    break;
  case 51:
    mForces.clear();
    mLinearVelocity = {0, 0, 0};
    mPlane->mPosition = {0, 0, 0};
    break;
  default:
    break;
  }
}

void ParticleScene::onScreenSizeChanged(glm::vec2 size) {
  mScreenSize = size;
  mCamera->mScreenSize = size;
}

void ParticleScene::updatePhysics(float delta) {
  mTime += delta;
  if (mass == 0) {
    return;
  }
  mForces.emplace_back(gravity * mass);
  glm::vec3 accumulatedForce = std::reduce(mForces.begin(), mForces.end());
  glm::vec3 acceleration = accumulatedForce / mass;
  mLinearVelocity += acceleration * delta;
  mPlane->mPosition += mLinearVelocity * delta;
  mForces.clear();
}

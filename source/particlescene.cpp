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

  for (int i = 0; i < number_of_particles; ++i) {
    auto shader = std::make_unique<DL::Shader>(
        "Shaders/particle.vert", "Shaders/particle.frag", mGlslVersionString);

    auto plane = std::make_unique<DL::Plane>(std::move(shader), *mCamera);
    plane->position = {0, 0, 0};
    plane->scale = {0.2, 0.2, 0.2};
    mPlanes.push_back(std::move(plane));
  }

  for (int i = 0; i < number_of_particles; ++i) {
    auto particle = std::make_unique<DL::Particle>(
        mPlanes[i]->position, 0.5, glm::vec3(0, -32.0f, 0));
    mParticles.push_back(std::move(particle));
  }

  mParticleSystem = std::make_unique<DL::ParticleSystem>();
  for (auto& p : mParticles) {
    mParticleSystem->addParticle(*p);
  }
}

void ParticleScene::render(float delta) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  mParticleSystem->updatePhysics(delta);

  for(auto& plane : mPlanes) {
    plane->render(delta);
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

void ParticleScene::onClick(double x, double y) {
  std::cout << "Mouse click @ x:" << x << " y:" << y << std::endl;
  mParticleSystem->explode();
}

void ParticleScene::onKey(int key) {
  std::cout << key << std::endl;
  glm::vec3 explode = {400, 800, 0};
  glm::vec3 dampen = {-200, 0, 0};
  switch (key) {
  case 49:
    mParticleSystem->reset();
    break;
  case 50:
    break;
  case 51:
    break;
  default:
    break;
  }
}

void ParticleScene::onScreenSizeChanged(glm::vec2 size) {
  mScreenSize = size;
  mCamera->mScreenSize = size;
}

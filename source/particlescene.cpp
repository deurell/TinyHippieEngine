#include "particlescene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <numeric>
#include <random>

ParticleScene::ParticleScene(std::string_view glslVersionString)
    : mGlslVersionString(glslVersionString) {}

void ParticleScene::init() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});

  initPlanes();
  initParticles();
  mParticleSystem = std::make_unique<DL::ParticleSystem>();
  for (auto &p : mParticles) {
    mParticleSystem->addParticle(*p);
  }
}
void ParticleScene::initParticles() {
  for (int i = 0; i < number_of_particles; ++i) {
    auto particle = std::make_unique<DL::Particle>(
        *mPlanes[i], 0.9, glm::vec3(0, -32.0f, 0));
    mParticles.emplace_back(std::move(particle));
  }
}

void ParticleScene::initPlanes() {
  for (int i = 0; i < number_of_particles; ++i) {
    auto shader = std::make_unique<DL::Shader>(
        "Shaders/particle.vert", "Shaders/particle.frag", mGlslVersionString);

    auto plane = std::make_unique<DL::Plane>(std::move(shader), *mCamera);
    plane->position = {0, 0, 0};
    plane->scale = {0.25, 0.25, 0.25};

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(-2.0, 2.0);
    glm::vec3 randomAxis = glm::vec3(dist(mt), dist(mt), dist(mt)*0.2);
    plane->rotationAxis = glm::normalize(randomAxis);
    plane->rotationSpeed = dist(mt) * glm::pi<float>()/180 * 360;

    mPlanes.emplace_back(std::move(plane));
  }
}

void ParticleScene::render(float delta) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  mParticleSystem->updatePhysics(delta);

  for (auto &plane : mPlanes) {
    plane->render(delta);
  }

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("fireworks scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif
}

void ParticleScene::onClick(double x, double y) {
  std::cout << "Mouse click @ x:" << x << " y:" << y << std::endl;
  glm::mat4 perspective = mCamera->getPerspectiveTransform();
  glm::vec4 viewport = glm::vec4(0, 0, 1024, 768);
  glm::vec3 screenPos = glm::vec3(x, 768-y, 1);
  glm::vec3 worldPos = glm::unProject(screenPos, mCamera->getViewMatrix(), perspective, viewport);
  worldPos.z = 0;

  mParticleSystem->explode(worldPos);
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
  default:
    break;
  }
}

void ParticleScene::onScreenSizeChanged(glm::vec2 size) {
  mScreenSize = size;
  mCamera->mScreenSize = size;
}

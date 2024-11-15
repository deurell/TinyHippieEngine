#include "particlescene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

ParticleScene::ParticleScene(std::string_view glslVersionString)
    : mGlslVersionString(glslVersionString) {
  std::random_device rd;
  twister = std::mt19937(rd());
}

void ParticleScene::init() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 36));
  mCamera->lookAt({0, 0, 0});
  initPlanes();
  initParticles();
  particleSystem_ = std::make_unique<DL::ParticleSystem>();
  for (auto &p : mParticles) {
    particleSystem_->addParticle(*p);
  }
}

void ParticleScene::update(float delta) {}

void ParticleScene::initParticles() {
  for (int i = 0; i < number_of_particles; ++i) {
    auto particle = std::make_unique<DL::Particle>(*mPlanes[i], 0.9,
                                                   glm::vec3(0, -32.0f, 0));
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

    std::uniform_real_distribution<double> dist(-2.0, 2.0);
    glm::vec3 randomAxis =
        glm::vec3(dist(twister), dist(twister), dist(twister) * 0.2);

    plane->rotationAxis = glm::normalize(randomAxis);
    plane->rotationSpeed = dist(twister) * glm::pi<float>() / 180 * 360;
    mPlanes.emplace_back(std::move(plane));
  }
}

void ParticleScene::render(float delta) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  particleSystem_->updatePhysics(delta);

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
  float mouseX = static_cast<float>(x);
  float mouseY = static_cast<float>(y);
  float invertedY = mCamera->mScreenSize.y - mouseY;
  glm::vec4 viewport(0.0f, 0.0f, mCamera->mScreenSize.x,
                     mCamera->mScreenSize.y);
  float desiredZ = 0.0f;

  glm::vec4 projected = mCamera->getPerspectiveTransform() *
                        mCamera->getViewMatrix() *
                        glm::vec4(0.0f, 0.0f, desiredZ, 1.0f);
  float ndcDepth = projected.z / projected.w;
  float depth = ndcDepth * 0.5f + 0.5f;
  glm::vec3 screenPos(mouseX, invertedY, depth);
  glm::vec3 worldPos =
      glm::unProject(screenPos, mCamera->getViewMatrix(),
                     mCamera->getPerspectiveTransform(), viewport);
  particleSystem_->explode(worldPos);
}

void ParticleScene::onKey(int key) {
  std::cout << key << std::endl;
  glm::vec3 explode = {400, 800, 0};
  glm::vec3 dampen = {-200, 0, 0};
  switch (key) {
  case 49:
    particleSystem_->reset();
    break;
  default:
    break;
  }
}

void ParticleScene::onScreenSizeChanged(glm::vec2 size) {
  mScreenSize = size;
  mCamera->mScreenSize = size;
}

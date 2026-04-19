#include "particlenodescene.h"

#include "debugui.h"
#ifdef USE_IMGUI
#include "imgui.h"
#endif
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

ParticleNodeScene::ParticleNodeScene(DL::IRenderDevice *renderDevice,
                                     DL::RenderResourceCache *renderResourceCache)
    : renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache) {}

namespace {

constexpr glm::vec3 defaultEmitterPosition{0.0f, 0.0f, 0.0f};

} // namespace

void ParticleNodeScene::init() {
  SceneNode::init();
  camera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 36.0f));
  camera_->lookAt({0.0f, 0.0f, 0.0f});

  const auto particleConfig = ParticleSystemNode::Config::waterFountain();

  
  auto particleSystemNode =
      std::make_unique<ParticleSystemNode>(renderDevice_, camera_.get(),
                                           renderResourceCache_,
                                           particleConfig, this);
  particleSystemNode->init();
  particleSystemNode->setEmitterPosition(defaultEmitterPosition);
  particleSystemNode_ = particleSystemNode.get();
  addChild(std::move(particleSystemNode));
}

void ParticleNodeScene::update(const DL::FrameContext &ctx) {
  SceneNode::update(ctx);
}

void ParticleNodeScene::render(const DL::FrameContext &ctx) {
  if (renderDevice_ != nullptr) {
    renderDevice_->beginFrame({.clearColor = {0.0f, 0.0f, 0.0f, 1.0f},
                               .clearFlags = DL::ClearFlags::ColorDepth,
                               .depthMode = DL::DepthMode::Disabled});
  }

  SceneNode::render(ctx);

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("Particle Node Scene");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  if (particleSystemNode_ != nullptr) {
    const auto &config = particleSystemNode_->getConfig();
    if (config.emission.mode == ParticleSystemNode::EmissionMode::Continuous) {
      ImGui::TextUnformatted("Click to move emitter");
    } else {
      ImGui::TextUnformatted("Click to trigger burst");
    }
    if (ImGui::Button("center emitter")) {
      particleSystemNode_->setEmitterPosition(defaultEmitterPosition);
      particleSystemNode_->resetParticles();
    }
  }
  ImGui::End();
#endif
}

void ParticleNodeScene::onClick(double x, double y) {
  if (particleSystemNode_ != nullptr) {
    const glm::vec3 worldPosition = screenToWorld(x, y, 0.0f);
    if (particleSystemNode_->getConfig().emission.mode ==
        ParticleSystemNode::EmissionMode::Continuous) {
      particleSystemNode_->setEmitterPosition(worldPosition);
    } else {
      particleSystemNode_->explode(worldPosition);
    }
  }
}

void ParticleNodeScene::onKey(int key) {
  if (key == GLFW_KEY_1 && particleSystemNode_ != nullptr) {
    particleSystemNode_->resetParticles();
    particleSystemNode_->setEmitterPosition(defaultEmitterPosition);
  }
}

void ParticleNodeScene::onScreenSizeChanged(glm::vec2 size) {
  screenSize_ = size;
  if (framebufferSize_.x == 0.0f || framebufferSize_.y == 0.0f) {
    framebufferSize_ = size;
  }
  if (camera_ != nullptr) {
    camera_->mScreenSize = framebufferSize_;
  }
  SceneNode::onScreenSizeChanged(size);
}

void ParticleNodeScene::onFramebufferSizeChanged(glm::vec2 size) {
  framebufferSize_ = size;
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

glm::vec3 ParticleNodeScene::screenToWorld(double x, double y, float worldZ) const {
  if (camera_ == nullptr || framebufferSize_.x <= 0.0f ||
      framebufferSize_.y <= 0.0f) {
    return {0.0f, 0.0f, worldZ};
  }

  glm::vec2 scale(1.0f, 1.0f);
  if (screenSize_.x > 0.0f && screenSize_.y > 0.0f) {
    scale.x = framebufferSize_.x / screenSize_.x;
    scale.y = framebufferSize_.y / screenSize_.y;
  }

  const float mouseX = static_cast<float>(x) * scale.x;
  const float mouseY = static_cast<float>(y) * scale.y;
  const glm::vec4 viewport(0.0f, 0.0f, framebufferSize_.x, framebufferSize_.y);
  const float viewportY = framebufferSize_.y - mouseY;

  const glm::mat4 view = camera_->getViewMatrix();
  const glm::mat4 projection = camera_->getPerspectiveTransform();
  const glm::vec3 nearPoint = glm::unProject(glm::vec3(mouseX, viewportY, 0.0f),
                                             view, projection, viewport);
  const glm::vec3 farPoint = glm::unProject(glm::vec3(mouseX, viewportY, 1.0f),
                                            view, projection, viewport);
  const glm::vec3 direction = farPoint - nearPoint;

  if (std::abs(direction.z) < 1e-5f) {
    return {nearPoint.x, nearPoint.y, worldZ};
  }

  const float distance = (worldZ - nearPoint.z) / direction.z;
  return nearPoint + direction * distance;
}

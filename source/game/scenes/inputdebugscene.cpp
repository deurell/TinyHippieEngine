#include "inputdebugscene.h"

#include "planenode.h"
#include "textnode.h"
#include <algorithm>
#include <iomanip>
#include <memory>
#include <sstream>
#include <utility>

namespace {
constexpr float kAxisPadRadius = 1.05f;
constexpr float kTextWorldScale = 0.004f;

std::string formatAxis(glm::vec2 axis) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << "moveAxis  x " << axis.x
         << "  y " << axis.y;
  return stream.str();
}
} // namespace

InputDebugScene::InputDebugScene(DL::IRenderDevice *renderDevice,
                                 DL::RenderResourceCache *renderResourceCache)
    : renderDevice_(renderDevice), renderResourceCache_(renderResourceCache) {}

void InputDebugScene::init() {
  setDebugName("input_debug_scene");
  camera_.mProjection = DL::CameraProjection::Orthographic;
  camera_.mOrthographicHeight = 6.0f;
  camera_.lookAt({0.0f, 0.0f, 0.0f});

  addPlane("axis_pad", {-1.35f, 0.0f, -0.08f}, {2.3f, 2.3f, 1.0f},
           {0.08f, 0.11f, 0.13f, 1.0f}, 0);
  addPlane("axis_horizontal", {-1.35f, 0.0f, -0.05f}, {2.0f, 0.035f, 1.0f},
           {0.42f, 0.52f, 0.55f, 1.0f}, 1);
  addPlane("axis_vertical", {-1.35f, 0.0f, -0.05f}, {0.035f, 2.0f, 1.0f},
           {0.42f, 0.52f, 0.55f, 1.0f}, 1);
  axisDot_ = addPlane("axis_dot", {-1.35f, 0.0f, 0.0f},
                      {0.22f, 0.22f, 1.0f},
                      {0.98f, 0.84f, 0.24f, 1.0f}, 2);

  addPlane("fire_socket", {1.6f, 0.0f, -0.08f}, {0.95f, 0.95f, 1.0f},
           {0.12f, 0.10f, 0.11f, 1.0f}, 0);
  fireIndicator_ = addPlane("fire_indicator", {1.6f, 0.0f, -0.02f},
                            {0.62f, 0.62f, 1.0f},
                            {0.38f, 0.12f, 0.15f, 1.0f}, 1);

  addText("Input Debug", {0.0f, 2.25f, 0.0f}, 34.0f);
  axisText_ = addText("moveAxis  x 0.00  y 0.00", {-1.35f, -1.55f, 0.0f},
                      21.0f);
  fireText_ = addText("Fire  up", {1.6f, -1.0f, 0.0f}, 23.0f);
  addText("left: move    right: A / Space", {0.0f, -2.35f, 0.0f}, 19.0f);

  SceneNode::init();
  for (auto &child : children) {
    child->init();
  }
}

void InputDebugScene::update(const DL::FrameContext &ctx) {
  updateReadout(ctx);
  SceneNode::update(ctx);
}

void InputDebugScene::render(const DL::FrameContext &ctx) {
  SceneNode::render(ctx);
}

void InputDebugScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
  camera_.mScreenSize = size;
}

void InputDebugScene::onFramebufferSizeChanged(glm::vec2 size) {
  framebufferSize_ = size;
  onScreenSizeChanged(size);
}

TextNode *InputDebugScene::addText(std::string text, glm::vec3 position,
                                   float pixelHeight) {
  auto node = std::make_unique<TextNode>(this, std::move(text), renderDevice_,
                                         renderResourceCache_, &camera_);
  node->setDebugName("input_debug_text");
  node->setLocalPosition(position);
  node->setLocalScale(
      {kTextWorldScale, kTextWorldScale, kTextWorldScale});
  node->setFontPixelHeight(pixelHeight);
  node->setTextColor({0.93f, 0.96f, 0.90f, 1.0f});
  node->setShadowColor({0.0f, 0.0f, 0.0f, 0.75f});
  node->setRenderLayer(10);
  auto *raw = node.get();
  addChild(std::move(node));
  return raw;
}

PlaneNode *InputDebugScene::addPlane(std::string name, glm::vec3 position,
                                     glm::vec3 scale, glm::vec4 color,
                                     int renderLayer) {
  auto node = std::make_unique<PlaneNode>(this, &camera_, renderDevice_,
                                          renderResourceCache_);
  node->setDebugName(std::move(name));
  node->setLocalPosition(position);
  node->setLocalScale(scale);
  node->color = color;
  node->setRenderLayer(renderLayer);
  auto *raw = node.get();
  addChild(std::move(node));
  return raw;
}

void InputDebugScene::updateReadout(const DL::FrameContext &ctx) {
  const glm::vec2 axis = glm::clamp(ctx.input.moveAxis, glm::vec2(-1.0f),
                                   glm::vec2(1.0f));
  if (axisDot_ != nullptr) {
    axisDot_->setLocalPosition(
        {-1.35f + axis.x * kAxisPadRadius, axis.y * kAxisPadRadius, 0.0f});
  }
  if (axisText_ != nullptr) {
    axisText_->setText(formatAxis(axis));
  }

  const bool fireDown = ctx.input.isActionDown(DL::Action::Fire);
  if (fireIndicator_ != nullptr) {
    fireIndicator_->color = fireDown ? glm::vec4(0.95f, 0.20f, 0.18f, 1.0f)
                                     : glm::vec4(0.38f, 0.12f, 0.15f, 1.0f);
    fireIndicator_->setLocalScale(fireDown ? glm::vec3(0.76f, 0.76f, 1.0f)
                                           : glm::vec3(0.62f, 0.62f, 1.0f));
  }
  if (fireText_ != nullptr) {
    fireText_->setText(fireDown ? "Fire  down" : "Fire  up");
  }
}

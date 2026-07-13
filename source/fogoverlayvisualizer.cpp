#include "fogoverlayvisualizer.h"

#include "fogoverlaynode.h"
#include "stb_image.h"
#include <iostream>
#include <utility>

DL::FogOverlayVisualizer::FogOverlayVisualizer(
    Camera &camera, FogOverlayNode &node, IRenderDevice *renderDevice,
    RenderResourceCache *resourceCache, std::string vertexShaderPath,
    std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(vertexShaderPath),
                     std::move(fragmentShaderPath), node),
      fogNode_(node), renderDevice_(renderDevice), resourceCache_(resourceCache) {
  if (renderDevice_ == nullptr) {
    return;
  }

  pipeline_ = resourceCache_ != nullptr
                  ? resourceCache_->acquirePipeline(vertexShaderPath_,
                                                    fragmentShaderPath_)
                  : renderDevice_->createPipeline(vertexShaderPath_,
                                                  fragmentShaderPath_);
  mesh_ = resourceCache_ != nullptr ? resourceCache_->acquireTexturedQuad()
                                    : renderDevice_->createTexturedQuad();

  const auto &config = fogNode_.config();
  if (resourceCache_ != nullptr) {
    if (const auto *resource =
            resourceCache_->acquireImageTexture(config.imagePath)) {
      texture_ = resource->texture;
      sharedTexture_ = true;
    }
  } else {
    stbi_set_flip_vertically_on_load(false);
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *pixels =
        stbi_load(config.imagePath.c_str(), &width, &height, &channels, 4);
    if (pixels != nullptr && width > 0 && height > 0) {
      texture_ = renderDevice_->createTexture(
          {.pixels = pixels,
           .width = static_cast<std::uint32_t>(width),
           .height = static_cast<std::uint32_t>(height),
           .format = TextureFormat::RGBA8,
           .filter = TextureFilter::Linear,
           .generateMipmaps = true});
    }
    stbi_image_free(pixels);
  }

  if (!texture_.valid()) {
    std::cerr << "Failed to load fog overlay texture: " << config.imagePath
              << "\n";
  }
}

DL::FogOverlayVisualizer::~FogOverlayVisualizer() {
  if (renderDevice_ == nullptr) {
    return;
  }
  if (mesh_.valid() && resourceCache_ == nullptr) {
    renderDevice_->destroy(mesh_);
  }
  if (texture_.valid() && !sharedTexture_) {
    renderDevice_->destroy(texture_);
  }
  if (pipeline_.valid() && resourceCache_ == nullptr) {
    renderDevice_->destroy(pipeline_);
  }
}

void DL::FogOverlayVisualizer::render(const glm::mat4 &worldTransform,
                                      const FrameContext &ctx,
                                      RenderPassId pass) {
  if (pass != RenderPassId::Opaque || renderDevice_ == nullptr ||
      !mesh_.valid() || !texture_.valid() || !pipeline_.valid()) {
    return;
  }

  const FogOverlayNode::Config &config = fogNode_.config();
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, extractPosition(worldTransform));
  model = model * glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform));

  DrawCommand command;
  command.mesh = mesh_;
  command.pipeline = pipeline_;
  command.texture = texture_;
  command.pass = pass;
  command.blendMode = BlendMode::Alpha;
  command.depthTest = false;
  command.sortMode = DrawSortMode::BackToFront;
  command.sortDepth = cameraDistanceSortDepth(extractPosition(worldTransform));
  command.uniforms.push_back(
      UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  command.uniforms.push_back(UniformValue::makeVec4("fogColor", config.color));
  command.uniforms.push_back(UniformValue::makeVec2("tiling", config.tiling));
  command.uniforms.push_back(
      UniformValue::makeVec2("scrollSpeed", config.scrollSpeed));
  command.uniforms.push_back(UniformValue::makeFloat("alpha", config.alpha));
  command.uniforms.push_back(
      UniformValue::makeFloat("softness", config.softness));
  command.uniforms.push_back(UniformValue::makeFloat(
      "secondLayerStrength", config.secondLayerStrength));
  command.uniforms.push_back(UniformValue::makeVec2(
      "secondLayerScrollSpeed", config.secondLayerScrollSpeed));
  command.uniforms.push_back(
      UniformValue::makeFloat("pulseAmount", config.pulseAmount));
  command.uniforms.push_back(
      UniformValue::makeFloat("pulseSpeed", config.pulseSpeed));
  command.uniforms.push_back(UniformValue::makeMat4("model", model));
  command.uniforms.push_back(
      UniformValue::makeMat4("view", camera_.getViewMatrix()));
  command.uniforms.push_back(UniformValue::makeMat4(
      "projection", camera_.getPerspectiveTransform()));
  renderDevice_->draw(command);
}

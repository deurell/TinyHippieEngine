#include "spritevisualizer.h"

#include <iostream>
#include <utility>
#include "stb_image.h"

namespace {

bool hasExtension(std::string_view path, std::string_view extension) {
  return path.size() >= extension.size() &&
         path.substr(path.size() - extension.size()) == extension;
}

} // namespace

DL::SpriteVisualizer::SpriteVisualizer(
    DL::Camera &camera, SceneNode &node, std::string texturePath,
    basist::etc1_global_selector_codebook *codeBook,
    DL::IRenderDevice *renderDevice, DL::RenderResourceCache *resourceCache,
    std::string vertexShaderPath, std::string fragmentShaderPath)
    : VisualizerBase(camera, vertexShaderPath, fragmentShaderPath, node),
      renderDevice_(renderDevice), texturePath_(std::move(texturePath)),
      codeBook_(codeBook), resourceCache_(resourceCache) {
  if (renderDevice_ == nullptr) {
    return;
  }

  pipeline_ = resourceCache_ != nullptr
                  ? resourceCache_->acquirePipeline(vertexShaderPath_,
                                                    fragmentShaderPath_)
                  : renderDevice_->createPipeline(vertexShaderPath,
                                                  fragmentShaderPath);
  mesh_ = resourceCache_ != nullptr ? resourceCache_->acquireTexturedQuad()
                                    : renderDevice_->createTexturedQuad();
  loadTexture();
}

DL::SpriteVisualizer::~SpriteVisualizer() {
  if (renderDevice_ != nullptr) {
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
}

void DL::SpriteVisualizer::render(const glm::mat4 &worldTransform,
                                  const DL::FrameContext &ctx,
                                  DL::RenderPassId pass) {
  if (pass != DL::RenderPassId::Opaque || renderDevice_ == nullptr ||
      !mesh_.valid() || !texture_.valid() || !pipeline_.valid()) {
    return;
  }

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, extractPosition(worldTransform));
  model = model * glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform));

  DL::DrawCommand command;
  command.mesh = mesh_;
  command.pipeline = pipeline_;
  command.texture = texture_;
  command.pass = pass;
  command.blendMode = BlendMode::Alpha;
  command.sortMode = DrawSortMode::BackToFront;
  const glm::vec3 spritePosition = extractPosition(worldTransform);
  command.sortDepth = cameraDistanceSortDepth(spritePosition);
  command.uniforms.push_back(
      DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  command.uniforms.push_back(DL::UniformValue::makeMat4("model", model));
  command.uniforms.push_back(
      DL::UniformValue::makeMat4("view", camera_.getViewMatrix()));
  command.uniforms.push_back(DL::UniformValue::makeMat4(
      "projection", camera_.getPerspectiveTransform()));
  renderDevice_->draw(command);
}

bool DL::SpriteVisualizer::loadTexture() {
  if (hasExtension(texturePath_, ".basis")) {
    if (codeBook_ == nullptr) {
      std::cerr << "Sprite texture needs a Basis codebook: " << texturePath_
                << "\n";
      return false;
    }

    texture_ = resourceCache_ != nullptr
                   ? resourceCache_->acquireBasisTexture(texturePath_, *codeBook_)
                   : renderDevice_->createBasisTexture(texturePath_, *codeBook_);
    sharedTexture_ = resourceCache_ != nullptr;
  } else {
    stbi_set_flip_vertically_on_load(false);
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *pixels =
        stbi_load(texturePath_.c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr || width <= 0 || height <= 0) {
      std::cerr << "Failed to load sprite texture: " << texturePath_ << "\n";
      if (pixels != nullptr) {
        stbi_image_free(pixels);
      }
      return false;
    }

    texture_ = renderDevice_->createTexture(
        {.pixels = pixels,
         .width = static_cast<std::uint32_t>(width),
         .height = static_cast<std::uint32_t>(height),
         .format = TextureFormat::RGBA8,
         .generateMipmaps = true});
    stbi_image_free(pixels);
    sharedTexture_ = false;
  }

  if (!texture_.valid()) {
    std::cerr << "Failed to load sprite texture: " << texturePath_ << "\n";
    return false;
  }
  return true;
}

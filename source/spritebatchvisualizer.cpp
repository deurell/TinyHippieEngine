#include "spritebatchvisualizer.h"

#include "renderqueue.h"
#include "scenenode.h"
#include "stb_image.h"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <utility>

namespace {

std::array<glm::vec2, 4> spriteUvs(const glm::vec4 &sourceRectPixels,
                                   const glm::vec2 &atlasSize, bool flipX,
                                   bool flipY, bool flipDiagonal) {
  glm::vec2 origin = sourceRectPixels.z > 0.0f && sourceRectPixels.w > 0.0f
                         ? glm::vec2(sourceRectPixels)
                         : glm::vec2(0.0f);
  glm::vec2 size = sourceRectPixels.z > 0.0f && sourceRectPixels.w > 0.0f
                       ? glm::vec2(sourceRectPixels.z, sourceRectPixels.w)
                       : atlasSize;

  std::array<glm::vec2, 4> local = {
      glm::vec2{1.0f, 0.0f}, glm::vec2{1.0f, 1.0f},
      glm::vec2{0.0f, 1.0f}, glm::vec2{0.0f, 0.0f}};
  for (glm::vec2 &uv : local) {
    if (flipDiagonal) {
      uv = {uv.y, uv.x};
    }
    if (flipX) {
      uv.x = 1.0f - uv.x;
    }
    if (flipY) {
      uv.y = 1.0f - uv.y;
    }
    uv = (origin + uv * size) / atlasSize;
  }
  return local;
}

} // namespace

namespace DL {

SpriteBatchVisualizer::SpriteBatchVisualizer(
    DL::Camera &camera, SceneNode &node, SpriteBatchConfig config,
    DL::IRenderDevice *renderDevice, DL::RenderResourceCache *resourceCache,
    std::string vertexShaderPath, std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(vertexShaderPath),
                     std::move(fragmentShaderPath), node),
      renderDevice_(renderDevice), resourceCache_(resourceCache),
      config_(std::move(config)) {
  if (renderDevice_ == nullptr) {
    return;
  }

  pipeline_ = resourceCache_ != nullptr
                  ? resourceCache_->acquirePipeline(vertexShaderPath_,
                                                    fragmentShaderPath_)
                  : renderDevice_->createPipeline(vertexShaderPath_,
                                                  fragmentShaderPath_);
  if (!pipeline_.valid()) {
    return;
  }

  if (resourceCache_ != nullptr) {
    if (const auto *resource =
            resourceCache_->acquireImageTexture(config_.imagePath)) {
      texture_ = resource->texture;
      atlasSize_ = resource->size;
      sharedTexture_ = true;
    }
  } else {
    stbi_set_flip_vertically_on_load(false);
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *pixels =
        stbi_load(config_.imagePath.c_str(), &width, &height, &channels, 4);
    if (pixels != nullptr && width > 0 && height > 0) {
      texture_ = renderDevice_->createTexture(
          {.pixels = pixels,
           .width = static_cast<std::uint32_t>(width),
           .height = static_cast<std::uint32_t>(height),
           .format = TextureFormat::RGBA8,
           .filter = TextureFilter::Nearest,
           .generateMipmaps = false});
      atlasSize_ = {static_cast<float>(width), static_cast<float>(height)};
    }
    stbi_image_free(pixels);
  }

  if (!texture_.valid()) {
    std::cerr << "Failed to load sprite batch texture: " << config_.imagePath
              << "\n";
    return;
  }

  buildMesh();
}

SpriteBatchVisualizer::~SpriteBatchVisualizer() {
  if (renderDevice_ == nullptr) {
    return;
  }
  if (mesh_.valid()) {
    renderDevice_->destroy(mesh_);
  }
  if (texture_.valid() && !sharedTexture_) {
    renderDevice_->destroy(texture_);
  }
  if (pipeline_.valid() && resourceCache_ == nullptr) {
    renderDevice_->destroy(pipeline_);
  }
}

void SpriteBatchVisualizer::buildMesh() {
  if (renderDevice_ == nullptr || atlasSize_.x <= 0.0f ||
      atlasSize_.y <= 0.0f || config_.sprites.empty()) {
    return;
  }

  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> uvs;
  std::vector<std::uint32_t> indices;
  positions.reserve(config_.sprites.size() * 4);
  normals.reserve(config_.sprites.size() * 4);
  uvs.reserve(config_.sprites.size() * 4);
  indices.reserve(config_.sprites.size() * 6);

  for (const SpriteBatchItem &sprite : config_.sprites) {
    const float radians = glm::radians(sprite.rotationDegrees);
    const float cosTheta = std::cos(radians);
    const float sinTheta = std::sin(radians);
    const glm::vec2 halfSize = sprite.size * 0.5f;
    const std::array<glm::vec2, 4> corners = {
        glm::vec2{halfSize.x, halfSize.y}, glm::vec2{halfSize.x, -halfSize.y},
        glm::vec2{-halfSize.x, -halfSize.y}, glm::vec2{-halfSize.x, halfSize.y}};
    const std::uint32_t base = static_cast<std::uint32_t>(positions.size());

    for (const glm::vec2 &corner : corners) {
      const glm::vec2 rotated{corner.x * cosTheta - corner.y * sinTheta,
                              corner.x * sinTheta + corner.y * cosTheta};
      positions.push_back(sprite.position + glm::vec3(rotated, 0.0f));
    }
    normals.insert(normals.end(), 4, glm::vec3(0.0f, 0.0f, 1.0f));

    const auto quadUvs =
        spriteUvs(sprite.sourceRectPixels, atlasSize_, sprite.flipX,
                  sprite.flipY, sprite.flipDiagonal);
    uvs.insert(uvs.end(), quadUvs.begin(), quadUvs.end());

    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 3);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 3);
  }

  if (!positions.empty()) {
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
    for (const glm::vec3 &position : positions) {
      minBounds = glm::min(minBounds, position);
      maxBounds = glm::max(maxBounds, position);
    }
    localBounds_.center = (minBounds + maxBounds) * 0.5f;
    localBounds_.halfExtents = (maxBounds - minBounds) * 0.5f;
  }

  mesh_ = renderDevice_->createMesh(positions, normals, uvs, indices);
}

void SpriteBatchVisualizer::render(const glm::mat4 &worldTransform,
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

  DL::RenderItem item;
  item.tag = RenderTag::SpriteBatch;
  item.renderLayer = node_.renderLayer();
  item.localBounds = localBounds_;
  item.mesh = mesh_;
  item.pipeline = pipeline_;
  item.texture = texture_;
  item.pass = pass;
  item.blendMode = BlendMode::Alpha;
  item.depthTest = false;
  item.sortMode = DrawSortMode::BackToFront;
  item.sortDepth = cameraDistanceSortDepth(extractPosition(worldTransform));
  item.uniforms.push_back(
      DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  item.uniforms.push_back(DL::UniformValue::makeMat4("model", model));
  item.uniforms.push_back(
      DL::UniformValue::makeMat4("view", camera_.getViewMatrix()));
  item.uniforms.push_back(DL::UniformValue::makeMat4(
      "projection", camera_.getPerspectiveTransform()));
  submitRenderItem(ctx, *renderDevice_, std::move(item));
}

} // namespace DL

#include "tilemaprendercomponent.h"

#include "renderqueue.h"
#include "scenenode.h"
#include "stb_image.h"
#include <array>
#include <iostream>
#include <limits>
#include <utility>

namespace {

std::array<glm::vec2, 4> transformedTileUvs(std::uint32_t tileIndex,
                                            std::uint32_t columns,
                                            std::uint32_t tileWidth,
                                            std::uint32_t tileHeight,
                                            const glm::vec2 &atlasSize,
                                            bool flipX, bool flipY,
                                            bool flipDiagonal) {
  const std::uint32_t col = columns > 0 ? tileIndex % columns : 0u;
  const std::uint32_t row = columns > 0 ? tileIndex / columns : 0u;
  const glm::vec2 origin{static_cast<float>(col * tileWidth),
                         static_cast<float>(row * tileHeight)};
  const glm::vec2 size{static_cast<float>(tileWidth),
                       static_cast<float>(tileHeight)};

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

TileMapRenderComponent::TileMapRenderComponent(
    DL::Camera &camera, SceneNode &node, TileMapConfig config,
    DL::IRenderDevice *renderDevice, DL::RenderResourceCache *resourceCache,
    std::string vertexShaderPath, std::string fragmentShaderPath)
    : RenderComponent(camera, std::move(vertexShaderPath),
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
    std::cerr << "Failed to load tilemap texture: " << config_.imagePath << "\n";
    return;
  }

  buildMesh();
}

TileMapRenderComponent::~TileMapRenderComponent() {
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

void TileMapRenderComponent::buildMesh() {
  if (renderDevice_ == nullptr || config_.columns == 0 ||
      config_.mapWidth == 0 || config_.mapHeight == 0 ||
      config_.tileWidth == 0 || config_.tileHeight == 0 ||
      atlasSize_.x <= 0.0f || atlasSize_.y <= 0.0f) {
    return;
  }

  const glm::vec2 atlasSize = atlasSize_;

  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> uvs;
  std::vector<std::uint32_t> indices;

  std::size_t tileCount = 0;
  for (const auto &layer : config_.layers) {
    tileCount += layer.tiles.size();
  }
  positions.reserve(tileCount * 4);
  normals.reserve(tileCount * 4);
  uvs.reserve(tileCount * 4);
  indices.reserve(tileCount * 6);

  const float halfTile = config_.tileWorldSize * 0.5f;
  const float centerX = (static_cast<float>(config_.mapWidth) - 1.0f) * 0.5f;
  const float centerY = (static_cast<float>(config_.mapHeight) - 1.0f) * 0.5f;

  for (const auto &layer : config_.layers) {
    for (const auto &tile : layer.tiles) {
      const float x =
          (static_cast<float>(tile.x) - centerX) * config_.tileWorldSize;
      const float y =
          (centerY - static_cast<float>(tile.y)) * config_.tileWorldSize;
      const float z = layer.z;
      const std::uint32_t base = static_cast<std::uint32_t>(positions.size());

      positions.push_back({x + halfTile, y + halfTile, z});
      positions.push_back({x + halfTile, y - halfTile, z});
      positions.push_back({x - halfTile, y - halfTile, z});
      positions.push_back({x - halfTile, y + halfTile, z});
      normals.insert(normals.end(), 4, glm::vec3(0.0f, 0.0f, 1.0f));

      const auto tileUvs =
          transformedTileUvs(tile.tileIndex, config_.columns, config_.tileWidth,
                             config_.tileHeight, atlasSize, tile.flipX,
                             tile.flipY, tile.flipDiagonal);
      uvs.insert(uvs.end(), tileUvs.begin(), tileUvs.end());

      indices.push_back(base + 0);
      indices.push_back(base + 1);
      indices.push_back(base + 3);
      indices.push_back(base + 1);
      indices.push_back(base + 2);
      indices.push_back(base + 3);
    }
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

void TileMapRenderComponent::render(const glm::mat4 &worldTransform,
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
  item.tag = RenderTag::TileMap;
  item.renderLayer = node_.renderLayer();
  item.localBounds = localBounds_;
  item.mesh = mesh_;
  item.pipeline = pipeline_;
  item.texture = texture_;
  item.pass = pass;
  item.blendMode = BlendMode::Alpha;
  item.depthTest = false;
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

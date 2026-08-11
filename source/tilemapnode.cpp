#include "tilemapnode.h"

#include <algorithm>
#include <cmath>
#include <utility>

TileMapNode::TileMapNode(DL::TileMapConfig config,
                         DL::IRenderDevice *renderDevice,
                         DL::RenderResourceCache *renderResourceCache,
                         DL::SceneNode *parentNode, DL::Camera *camera)
    : SceneNode(parentNode), config_(std::move(config)), camera_(camera),
      renderDevice_(renderDevice), renderResourceCache_(renderResourceCache) {}

void TileMapNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void TileMapNode::update(const DL::FrameContext &ctx) {
  rebuildRenderDataIfDirty();
  SceneNode::update(ctx);
}

void TileMapNode::render(const DL::FrameContext &ctx) {
  SceneNode::render(ctx);
}

void TileMapNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void TileMapNode::initCamera() {
  if (camera_ != nullptr) {
    return;
  }
  localCamera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 20.0f));
  localCamera_->lookAt({0.0f, 0.0f, 0.0f});
  camera_ = localCamera_.get();
}

void TileMapNode::initComponents() {
  if (camera_ == nullptr || renderDevice_ == nullptr) {
    return;
  }

  auto component = std::make_unique<DL::TileMapRenderComponent>(
      *camera_, *this, config_, renderDevice_, renderResourceCache_);
  renderComponent_ = component.get();
  addRenderComponent(std::move(component));
  renderDataDirty_ = false;
}

std::optional<DL::TileMapCell> TileMapNode::tile(std::string_view layerName,
                                                 glm::ivec2 cell) const {
  const DL::TileMapLayer *layer = findLayer(layerName);
  if (layer == nullptr || !containsCell(cell)) {
    return std::nullopt;
  }
  const auto found =
      std::ranges::find_if(layer->tiles, [cell](const DL::TileMapTile &tile) {
        return tile.x == static_cast<std::uint32_t>(cell.x) &&
               tile.y == static_cast<std::uint32_t>(cell.y);
      });
  if (found == layer->tiles.end()) {
    return std::nullopt;
  }
  return DL::TileMapCell{.tileIndex = found->tileIndex,
                         .flipX = found->flipX,
                         .flipY = found->flipY,
                         .flipDiagonal = found->flipDiagonal};
}

bool TileMapNode::setTile(std::string_view layerName, glm::ivec2 cell,
                          const DL::TileMapCell &cellValue) {
  DL::TileMapLayer *layer = findLayer(layerName);
  if (layer == nullptr || !containsCell(cell)) {
    return false;
  }
  auto found =
      std::ranges::find_if(layer->tiles, [cell](const DL::TileMapTile &tile) {
        return tile.x == static_cast<std::uint32_t>(cell.x) &&
               tile.y == static_cast<std::uint32_t>(cell.y);
      });
  const DL::TileMapTile value{.tileIndex = cellValue.tileIndex,
                              .x = static_cast<std::uint32_t>(cell.x),
                              .y = static_cast<std::uint32_t>(cell.y),
                              .flipX = cellValue.flipX,
                              .flipY = cellValue.flipY,
                              .flipDiagonal = cellValue.flipDiagonal};
  if (found == layer->tiles.end()) {
    layer->tiles.push_back(value);
  } else {
    *found = value;
  }
  renderDataDirty_ = true;
  return true;
}

bool TileMapNode::clearTile(std::string_view layerName, glm::ivec2 cell) {
  DL::TileMapLayer *layer = findLayer(layerName);
  if (layer == nullptr || !containsCell(cell)) {
    return false;
  }
  const auto previousSize = layer->tiles.size();
  std::erase_if(layer->tiles, [cell](const DL::TileMapTile &tile) {
    return tile.x == static_cast<std::uint32_t>(cell.x) &&
           tile.y == static_cast<std::uint32_t>(cell.y);
  });
  const bool changed = layer->tiles.size() != previousSize;
  renderDataDirty_ = renderDataDirty_ || changed;
  return changed;
}

glm::vec2 TileMapNode::mapToLocal(glm::ivec2 cell) const {
  const float centerX = (static_cast<float>(config_.mapWidth) - 1.0f) * 0.5f;
  const float centerY = (static_cast<float>(config_.mapHeight) - 1.0f) * 0.5f;
  return {(static_cast<float>(cell.x) - centerX) * config_.tileWorldSize,
          (centerY - static_cast<float>(cell.y)) * config_.tileWorldSize};
}

std::optional<glm::ivec2>
TileMapNode::localToMap(glm::vec2 localPosition) const {
  if (config_.tileWorldSize <= 0.0f || config_.mapWidth == 0u ||
      config_.mapHeight == 0u) {
    return std::nullopt;
  }
  const glm::ivec2 cell{
      static_cast<int>(std::floor(localPosition.x / config_.tileWorldSize +
                                  static_cast<float>(config_.mapWidth) * 0.5f)),
      static_cast<int>(std::floor(static_cast<float>(config_.mapHeight) * 0.5f -
                                  localPosition.y / config_.tileWorldSize))};
  return containsCell(cell) ? std::optional<glm::ivec2>(cell) : std::nullopt;
}

bool TileMapNode::containsCell(glm::ivec2 cell) const {
  return cell.x >= 0 && cell.y >= 0 &&
         cell.x < static_cast<int>(config_.mapWidth) &&
         cell.y < static_cast<int>(config_.mapHeight);
}

DL::TileMapLayer *TileMapNode::findLayer(std::string_view name) {
  const auto found =
      std::ranges::find(config_.layers, name, &DL::TileMapLayer::name);
  return found != config_.layers.end() ? &*found : nullptr;
}

const DL::TileMapLayer *TileMapNode::findLayer(std::string_view name) const {
  const auto found =
      std::ranges::find(config_.layers, name, &DL::TileMapLayer::name);
  return found != config_.layers.end() ? &*found : nullptr;
}

void TileMapNode::rebuildRenderDataIfDirty() {
  if (!renderDataDirty_ || renderComponent_ == nullptr) {
    return;
  }
  renderComponent_->setConfig(config_);
  renderDataDirty_ = false;
}

#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include "tilemaprendercomponent.h"
#include <optional>
#include <string>
#include <string_view>

class TileMapNode : public DL::SceneNode {
public:
  TileMapNode(DL::TileMapConfig config, DL::IRenderDevice *renderDevice,
              DL::RenderResourceCache *renderResourceCache = nullptr,
              DL::SceneNode *parentNode = nullptr,
              DL::Camera *camera = nullptr);
  ~TileMapNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "TileMapNode";
  }
  [[nodiscard]] const DL::TileMapConfig &config() const { return config_; }
  [[nodiscard]] std::optional<DL::TileMapCell> tile(std::string_view layer,
                                                    glm::ivec2 cell) const;
  bool setTile(std::string_view layer, glm::ivec2 cell,
               const DL::TileMapCell &tile);
  bool clearTile(std::string_view layer, glm::ivec2 cell);
  [[nodiscard]] glm::vec2 mapToLocal(glm::ivec2 cell) const;
  [[nodiscard]] std::optional<glm::ivec2>
  localToMap(glm::vec2 localPosition) const;

private:
  void initCamera();
  void initComponents();
  void rebuildRenderDataIfDirty();
  [[nodiscard]] bool containsCell(glm::ivec2 cell) const;
  DL::TileMapLayer *findLayer(std::string_view name);
  [[nodiscard]] const DL::TileMapLayer *findLayer(std::string_view name) const;

  DL::TileMapConfig config_;
  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_ = nullptr;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  DL::TileMapRenderComponent *renderComponent_ = nullptr;
  bool renderDataDirty_ = false;
};

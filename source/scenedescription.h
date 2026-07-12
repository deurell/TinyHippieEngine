#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "meshassetcache.h"
#include "meshvisualizer.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include "shapevisualizer.h"
#include "tilemapvisualizer.h"
#include <filesystem>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace DL {

struct SceneAnimationDescription {
  std::string clip;
  bool playing = true;
  bool looping = true;
  float playbackSpeed = 1.0f;
};

struct SceneNodeDescription {
  std::string name = "node";
  std::string type = "SceneNode";
  glm::vec3 position{0.0f};
  glm::vec3 rotationEulerDegrees{0.0f};
  glm::vec3 scale{1.0f};
  std::string mesh;
  std::string image;
  glm::vec4 sourceRect{0.0f, 0.0f, -1.0f, -1.0f};
  std::string text = "text";
  std::string plane = "Simple";
  std::string shape = "Cube";
  std::string particle = "SoftGlowBurst";
  std::string textAlignment = "Center";
  std::string textAnchor = "Center";
  float fontSize = 48.0f;
  glm::vec4 textColor{1.0f, 1.0f, 1.0f, 1.0f};
  glm::vec4 shadowColor{0.0f, 0.0f, 0.0f, 0.58f};
  glm::vec2 shadowOffset{1.5f, -1.5f};
  bool billboard = false;
  bool flipX = false;
  bool flipY = false;
  bool flipDiagonal = false;
  bool active = false;
  float fov = 45.0f;
  std::optional<glm::vec3> lookAt;
  glm::vec4 color{0.9f, 0.9f, 0.9f, 1.0f};
  PhongMaterial material;
  MeshVisualizerSettings visualizerSettings;
  TileMapConfig tileMap;
  std::optional<SceneAnimationDescription> animation;
  std::vector<SceneNodeDescription> children;
};

struct SceneDescription {
  std::string name = "scene";
  std::vector<SceneNodeDescription> nodes;
};

struct SceneBuilderContext {
  IRenderDevice *renderDevice = nullptr;
  basist::etc1_global_selector_codebook *codeBook = nullptr;
  MeshAssetCache *meshAssetCache = nullptr;
  RenderResourceCache *renderResourceCache = nullptr;
  Camera *camera = nullptr;
};

class SceneNodeFactory {
public:
  using Builder = std::function<std::unique_ptr<SceneNode>(
      const SceneNodeDescription &, SceneBuilderContext, SceneNode *)>;

  void registerNodeType(std::string type, Builder builder);
  std::unique_ptr<SceneNode> build(const SceneNodeDescription &description,
                                   SceneBuilderContext context,
                                   SceneNode *parent = nullptr) const;
  [[nodiscard]] bool hasNodeType(std::string_view type) const;

private:
  std::unordered_map<std::string, Builder> builders_;
};

SceneNodeFactory createDefaultSceneNodeFactory();

SceneDescription loadSceneDescription(const std::filesystem::path &path);
SceneDescription parseSceneDescription(std::string_view source,
                                        std::string_view sourceName);

std::unique_ptr<SceneNode> buildSceneNode(const SceneNodeDescription &description,
                                          SceneBuilderContext context,
                                          SceneNode *parent = nullptr);

SceneNode *findSceneNodeByName(SceneNode &root, std::string_view name);

} // namespace DL

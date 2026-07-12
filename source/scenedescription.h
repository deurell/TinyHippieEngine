#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "meshassetcache.h"
#include "meshvisualizer.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <filesystem>
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
  MeshVisualizerSettings visualizerSettings;
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

SceneDescription loadSceneDescription(const std::filesystem::path &path);
SceneDescription parseSceneDescription(std::string_view source,
                                        std::string_view sourceName);

std::unique_ptr<SceneNode> buildSceneNode(const SceneNodeDescription &description,
                                          SceneBuilderContext context,
                                          SceneNode *parent = nullptr);

SceneNode *findSceneNodeByName(SceneNode &root, std::string_view name);

} // namespace DL

#pragma once

#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace Deflektorish {

struct GridConfig {
  float tileSize = 32.0f;
  glm::vec2 origin{16.0f, 16.0f};
};

struct SourceConfig {
  glm::ivec2 cell{0};
  float angleDegrees = 0.0f;
};

struct ReflektorConfig {
  glm::ivec2 cell{0};
  float angleDegrees = 0.0f;
  bool automatic = false;
  float speed = 0.0f;
};

struct TargetConfig {
  glm::ivec2 cell{0};
};

struct BlockerConfig {
  glm::ivec2 cell{0};
  bool reflective = false;
};

struct PortalConfig {
  glm::ivec2 entryCell{0};
  glm::ivec2 exitCell{0};
  float phase = 0.0f;
};

struct FilterConfig {
  glm::ivec2 cell{0};
  float angleDegrees = 0.0f;
  bool automatic = false;
  float speed = 0.0f;
};

struct SplitterConfig {
  glm::ivec2 cell{0};
  float angleDegrees = 0.0f;
};

struct LevelConfig {
  std::string name = "deflektorish_level";
  GridConfig grid;
  SourceConfig source;
  int explosionPoolSize = 12;
  std::vector<ReflektorConfig> reflektors;
  std::vector<TargetConfig> targets;
  std::vector<BlockerConfig> blockers;
  std::vector<PortalConfig> portals;
  std::vector<FilterConfig> filters;
  std::vector<SplitterConfig> splitters;
};

glm::vec2 cellToPosition(const GridConfig &grid, glm::ivec2 cell);
LevelConfig parseLevel(std::string_view source, std::string_view sourceName);
LevelConfig loadLevel(const std::filesystem::path &path);

} // namespace Deflektorish

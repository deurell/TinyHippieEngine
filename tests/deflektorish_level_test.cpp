#include "game/deflektorish/deflektorishlevel.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>

namespace {

TEST(DeflektorishLevelTest, ParsesMinimalLevelData) {
  constexpr char kSource[] = R"json({
    "name": "unit_level",
    "grid": { "tileSize": 32, "origin": [16, 16] },
    "source": { "cell": [3, 7], "angleDegrees": 0 },
    "explosionPoolSize": 4,
    "reflektors": [
      { "cell": [12, 7], "angleDegrees": 45, "automatic": false },
      { "cell": [8, 12], "angleDegrees": 45, "automatic": true, "speed": 0.4 }
    ],
    "targets": [{ "cell": [6, 3] }],
    "blockers": [{ "cell": [4, 3], "reflective": false }],
    "portals": [{ "entryCell": [16, 3], "exitCell": [22, 11], "phase": 0.2 }],
    "filters": [{ "cell": [12, 10], "angleDegrees": 90, "automatic": false }],
    "splitters": [{ "cell": [16, 7], "angleDegrees": 0 }]
  })json";

  const Deflektorish::LevelConfig level =
      Deflektorish::parseLevel(kSource, "unit");

  EXPECT_EQ(level.name, "unit_level");
  EXPECT_EQ(level.explosionPoolSize, 4);
  EXPECT_EQ(level.source.cell, glm::ivec2(3, 7));
  EXPECT_EQ(level.reflektors.size(), 2u);
  EXPECT_TRUE(level.reflektors[1].automatic);
  EXPECT_FLOAT_EQ(level.reflektors[1].speed, 0.4f);
  EXPECT_EQ(level.targets.size(), 1u);
  EXPECT_EQ(level.blockers.size(), 1u);
  EXPECT_FALSE(level.blockers[0].reflective);
  EXPECT_EQ(level.portals[0].exitCell, glm::ivec2(22, 11));
  EXPECT_EQ(Deflektorish::cellToPosition(level.grid, {3, 7}),
            glm::vec2(112.0f, 240.0f));
}

TEST(DeflektorishLevelTest, LoadsDefaultLevelFile) {
  std::filesystem::path path =
      "../Resources/Game/Deflektorish/Levels/level_01.json";
  if (!std::filesystem::exists(path)) {
    path = "Resources/Game/Deflektorish/Levels/level_01.json";
  }

  const Deflektorish::LevelConfig level = Deflektorish::loadLevel(path);

  EXPECT_EQ(level.name, "level_01");
  EXPECT_EQ(level.reflektors.size(), 3u);
  EXPECT_EQ(level.targets.size(), 32u);
  EXPECT_EQ(level.blockers.size(), 62u);
  EXPECT_EQ(level.portals.size(), 1u);
  EXPECT_EQ(level.filters.size(), 2u);
  EXPECT_EQ(level.splitters.size(), 1u);
}

TEST(DeflektorishLevelTest, RejectsUnknownFields) {
  constexpr char kSource[] = R"json({
    "name": "bad_level",
    "source": { "cell": [0, 0] },
    "reflektors": [{ "cell": [1, 1], "angleDegrees": 0, "mystery": true }]
  })json";

  EXPECT_THROW((void)Deflektorish::parseLevel(kSource, "bad"), std::runtime_error);
}

} // namespace

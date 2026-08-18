#include "game/deflektorish/deflektorishlevel.h"
#include "game/deflektorish/beamworld.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <gtest/gtest.h>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

bool hasFreeTargetBeforeFirstInteraction(const Deflektorish::LevelConfig &level) {
  const glm::ivec2 source = level.source.cell;
  int firstInteractionX = std::numeric_limits<int>::max();

  auto consider = [&](glm::ivec2 cell) {
    if (cell.y == source.y && cell.x > source.x) {
      firstInteractionX = std::min(firstInteractionX, cell.x);
    }
  };

  for (const Deflektorish::ReflektorConfig &reflektor : level.reflektors) {
    consider(reflektor.cell);
  }
  for (const Deflektorish::BlockerConfig &blocker : level.blockers) {
    consider(blocker.cell);
  }
  for (const Deflektorish::PortalConfig &portal : level.portals) {
    consider(portal.entryCell);
  }
  for (const Deflektorish::FilterConfig &filter : level.filters) {
    consider(filter.cell);
  }
  for (const Deflektorish::SplitterConfig &splitter : level.splitters) {
    consider(splitter.cell);
  }

  if (firstInteractionX == std::numeric_limits<int>::max()) {
    return false;
  }

  for (const Deflektorish::TargetConfig &target : level.targets) {
    if (target.cell.y == source.y && target.cell.x > source.x &&
        target.cell.x < firstInteractionX) {
      return true;
    }
  }
  return false;
}

Deflektorish::BeamWorld makeWorld(const Deflektorish::LevelConfig &level) {
  Deflektorish::BeamWorld world;
  world.sourcePosition = Deflektorish::cellToPosition(level.grid, level.source.cell);
  world.sourceAngle = level.source.angleDegrees * 3.1415926535f / 180.0f;
  for (const Deflektorish::ReflektorConfig &reflektor : level.reflektors) {
    world.reflektors.push_back(
        {Deflektorish::cellToPosition(level.grid, reflektor.cell),
         reflektor.angleDegrees * 3.1415926535f / 180.0f});
  }
  for (const Deflektorish::TargetConfig &target : level.targets) {
    world.targets.push_back(
        {Deflektorish::cellToPosition(level.grid, target.cell), true});
  }
  for (const Deflektorish::BlockerConfig &blocker : level.blockers) {
    world.blockers.push_back(
        {Deflektorish::cellToPosition(level.grid, blocker.cell),
         blocker.reflective});
  }
  for (const Deflektorish::PortalConfig &portal : level.portals) {
    world.portals.push_back(
        {Deflektorish::cellToPosition(level.grid, portal.entryCell),
         Deflektorish::cellToPosition(level.grid, portal.exitCell)});
  }
  for (const Deflektorish::FilterConfig &filter : level.filters) {
    world.filters.push_back(
        {Deflektorish::cellToPosition(level.grid, filter.cell),
         filter.angleDegrees * 3.1415926535f / 180.0f});
  }
  for (const Deflektorish::SplitterConfig &splitter : level.splitters) {
    world.splitters.push_back(
        {Deflektorish::cellToPosition(level.grid, splitter.cell),
         splitter.angleDegrees * 3.1415926535f / 180.0f});
  }
  return world;
}

std::vector<bool> clearTargetMask(Deflektorish::BeamWorld world) {
  std::vector<bool> cleared(world.targets.size(), false);
  for (std::size_t step = 0; step < world.targets.size() + 6; ++step) {
    const Deflektorish::BeamSolveResult result =
        Deflektorish::solveBeamWorld(world, 64);
    bool changed = false;
    for (std::size_t i = 0; i < result.hitTargets.size(); ++i) {
      if (result.hitTargets[i] && world.targets[i].alive) {
        world.targets[i].alive = false;
        cleared[i] = true;
        changed = true;
      }
    }
    if (!changed) {
      break;
    }
  }
  return cleared;
}

std::size_t clearTargets(Deflektorish::BeamWorld world) {
  const std::vector<bool> cleared = clearTargetMask(std::move(world));
  return static_cast<std::size_t>(
      std::count(cleared.begin(), cleared.end(), true));
}

std::string missedTargetsText(const Deflektorish::LevelConfig &level,
                              const std::vector<bool> &cleared) {
  std::string text;
  for (std::size_t i = 0; i < level.targets.size(); ++i) {
    if (i < cleared.size() && cleared[i]) {
      continue;
    }
    const glm::ivec2 cell = level.targets[i].cell;
    text += " [" + std::to_string(cell.x) + "," + std::to_string(cell.y) + "]";
  }
  return text;
}

std::filesystem::path levelPath(int i) {
  std::string index = std::to_string(i);
  if (i < 10) {
    index.insert(index.begin(), '0');
  }
  std::filesystem::path path =
      "../Resources/Game/Deflektorish/Levels/level_" + index + ".json";
  if (!std::filesystem::exists(path)) {
    path = "Resources/Game/Deflektorish/Levels/level_" + index + ".json";
  }
  return path;
}

TEST(DeflektorishLevelTest, ParsesMinimalLevelData) {
  constexpr char kSource[] = R"json({
    "name": "unit_level",
    "parTimeSeconds": 42.5,
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
    "splitters": [{ "cell": [16, 7], "angleDegrees": 0 }],
    "enemies": [{ "spawnCell": [2, 12], "targetReflektorIndex": 0,
                   "spawnDelay": 4, "spawnInterval": 8, "speed": 28,
                   "destroySeconds": 7.5, "maxSpawns": 9 }]
  })json";

  const Deflektorish::LevelConfig level =
      Deflektorish::parseLevel(kSource, "unit");

  EXPECT_EQ(level.name, "unit_level");
  EXPECT_FLOAT_EQ(level.parTimeSeconds, 42.5f);
  EXPECT_EQ(level.explosionPoolSize, 4);
  EXPECT_EQ(level.source.cell, glm::ivec2(3, 7));
  EXPECT_EQ(level.reflektors.size(), 2u);
  EXPECT_TRUE(level.reflektors[1].automatic);
  EXPECT_FLOAT_EQ(level.reflektors[1].speed, 0.4f);
  EXPECT_EQ(level.targets.size(), 1u);
  EXPECT_EQ(level.blockers.size(), 1u);
  EXPECT_FALSE(level.blockers[0].reflective);
  EXPECT_EQ(level.portals[0].exitCell, glm::ivec2(22, 11));
  ASSERT_EQ(level.enemies.size(), 1u);
  EXPECT_EQ(level.enemies[0].targetReflektorIndex, 0u);
  EXPECT_FLOAT_EQ(level.enemies[0].spawnDelay, 4.0f);
  EXPECT_FLOAT_EQ(level.enemies[0].spawnInterval, 8.0f);
  EXPECT_FLOAT_EQ(level.enemies[0].destroySeconds, 7.5f);
  EXPECT_EQ(level.enemies[0].maxSpawns, 9);
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

  EXPECT_EQ(level.name, "level_01_chicane");
  EXPECT_EQ(level.reflektors.size(), 4u);
  EXPECT_EQ(level.targets.size(), 8u);
  EXPECT_EQ(level.blockers.size(), 14u);
}

TEST(DeflektorishLevelTest, LoadsProgressionLevelFiles) {
  constexpr float kExpectedParTimes[] = {
      15.0f, 15.0f, 28.0f, 34.0f, 38.0f, 42.0f, 46.0f, 50.0f, 44.0f, 58.0f,
      40.0f, 38.0f, 58.0f, 50.0f, 48.0f, 42.0f, 56.0f, 60.0f, 68.0f, 80.0f};
  for (int i = 1; i <= 20; ++i) {
    const std::filesystem::path path = levelPath(i);

    const Deflektorish::LevelConfig level = Deflektorish::loadLevel(path);

    EXPECT_FALSE(level.name.empty()) << path;
    EXPECT_FLOAT_EQ(level.parTimeSeconds, kExpectedParTimes[i - 1]) << path;
    EXPECT_FALSE(level.reflektors.empty()) << path;
    EXPECT_FALSE(level.targets.empty()) << path;
    if (i <= 2) {
      EXPECT_FALSE(hasFreeTargetBeforeFirstInteraction(level)) << path;
    }
  }
}

TEST(DeflektorishLevelTest, RejectsNonPositiveParTime) {
  constexpr char kSource[] = R"json({
    "name": "bad_par",
    "parTimeSeconds": 0
  })json";

  EXPECT_THROW((void)Deflektorish::parseLevel(kSource, "bad"),
               std::runtime_error);
}

TEST(DeflektorishLevelTest, TrainingLevelsNeedInteractionAndHaveSolutions) {
  const std::vector<std::vector<float>> solutions = {
      {45.0f, 45.0f, -45.0f, -45.0f},
      {45.0f, 45.0f, 45.0f, -45.0f},
  };

  for (int i = 1; i <= 2; ++i) {
    Deflektorish::LevelConfig level = Deflektorish::loadLevel(levelPath(i));
    EXPECT_LT(clearTargets(makeWorld(level)), level.targets.size())
        << level.name << " should not clear itself with authored defaults";

    ASSERT_EQ(level.reflektors.size(), solutions[static_cast<std::size_t>(i - 1)].size())
        << level.name;
    for (std::size_t reflektor = 0; reflektor < level.reflektors.size();
         ++reflektor) {
      level.reflektors[reflektor].angleDegrees =
          solutions[static_cast<std::size_t>(i - 1)][reflektor];
    }
    const std::vector<bool> cleared = clearTargetMask(makeWorld(level));
    EXPECT_EQ(std::count(cleared.begin(), cleared.end(), true),
              static_cast<int>(level.targets.size()))
        << level.name << " intended solution should clear every target; missed"
        << missedTargetsText(level, cleared);
  }
}

TEST(DeflektorishLevelTest, LaterProgressionLevelsRemainInteractivePuzzles) {
  for (int i = 3; i <= 20; ++i) {
    const Deflektorish::LevelConfig level = Deflektorish::loadLevel(levelPath(i));

    EXPECT_GE(level.reflektors.size(), 5u) << level.name;
    EXPECT_GE(level.targets.size(), 6u) << level.name;
    EXPECT_LT(clearTargets(makeWorld(level)), level.targets.size())
        << level.name << " should not clear itself with authored defaults";
  }
}

TEST(DeflektorishLevelTest, ProgressionIncludesCrowdedMirrorLayouts) {
  int crowdedLayouts = 0;
  for (int i = 3; i <= 20; ++i) {
    const Deflektorish::LevelConfig level = Deflektorish::loadLevel(levelPath(i));
    int closePairs = 0;
    for (std::size_t a = 0; a < level.reflektors.size(); ++a) {
      for (std::size_t b = a + 1; b < level.reflektors.size(); ++b) {
        const glm::ivec2 delta = glm::abs(level.reflektors[a].cell -
                                          level.reflektors[b].cell);
        if (delta.x + delta.y <= 3) {
          ++closePairs;
        }
      }
    }
    if (closePairs >= 4) {
      ++crowdedLayouts;
    }
  }
  EXPECT_GE(crowdedLayouts, 5);
}

TEST(DeflektorishLevelTest, MirrorMazeUsesContiguousReflectiveWalls) {
  const Deflektorish::LevelConfig level = Deflektorish::loadLevel(levelPath(13));
  const auto reflectiveCount =
      std::count_if(level.blockers.begin(), level.blockers.end(),
                    [](const auto &blocker) { return blocker.reflective; });
  int adjacentWallPairs = 0;
  for (std::size_t a = 0; a < level.blockers.size(); ++a) {
    if (!level.blockers[a].reflective) continue;
    for (std::size_t b = a + 1; b < level.blockers.size(); ++b) {
      if (!level.blockers[b].reflective) continue;
      const glm::ivec2 delta =
          glm::abs(level.blockers[a].cell - level.blockers[b].cell);
      if (delta.x + delta.y == 1) ++adjacentWallPairs;
    }
  }
  EXPECT_GE(reflectiveCount, 60);
  EXPECT_GE(adjacentWallPairs, 50);
  EXPECT_TRUE(level.splitters.empty());
}

TEST(DeflektorishLevelTest, ActionBoardsDoNotMostlySolveFromDefaults) {
  for (int i = 3; i <= 20; ++i) {
    const Deflektorish::LevelConfig level = Deflektorish::loadLevel(levelPath(i));
    const std::size_t initiallyCleared = clearTargets(makeWorld(level));
    EXPECT_LE(initiallyCleared, std::max<std::size_t>(2, level.targets.size() / 3))
        << level.name << " clears " << initiallyCleared << " of "
        << level.targets.size() << " targets without deliberate aiming";
  }
}

TEST(DeflektorishLevelTest, AuthoredObjectsNeverShareCells) {
  for (int i = 1; i <= 20; ++i) {
    const Deflektorish::LevelConfig level = Deflektorish::loadLevel(levelPath(i));
    std::set<std::pair<int, int>> occupied;
    auto claim = [&](glm::ivec2 cell, const char *kind) {
      EXPECT_TRUE(occupied.emplace(cell.x, cell.y).second)
          << level.name << " has overlapping " << kind << " at [" << cell.x
          << ',' << cell.y << ']';
    };
    claim(level.source.cell, "source");
    for (const auto &item : level.reflektors) claim(item.cell, "reflektor");
    for (const auto &item : level.targets) claim(item.cell, "target");
    for (const auto &item : level.blockers) claim(item.cell, "blocker");
    for (const auto &item : level.filters) claim(item.cell, "filter");
    for (const auto &item : level.splitters) claim(item.cell, "splitter");
    for (const auto &item : level.enemies) claim(item.spawnCell, "nest");
    for (const auto &item : level.portals) {
      claim(item.entryCell, "portal entry");
      claim(item.exitCell, "portal exit");
    }
  }
}

TEST(DeflektorishLevelTest, LaterLevelsContainAnAutomaticOptic) {
  for (int i = 3; i <= 20; ++i) {
    const Deflektorish::LevelConfig level = Deflektorish::loadLevel(levelPath(i));
    const auto hasAutomaticReflektor =
        std::any_of(level.reflektors.begin(), level.reflektors.end(),
                    [](const Deflektorish::ReflektorConfig &reflektor) {
                      return reflektor.automatic && reflektor.speed != 0.0f;
                    });
    const auto hasAutomaticFilter =
        std::any_of(level.filters.begin(), level.filters.end(),
                    [](const Deflektorish::FilterConfig &filter) {
                      return filter.automatic && filter.speed != 0.0f;
                    });

    EXPECT_TRUE(hasAutomaticReflektor || hasAutomaticFilter) << level.name;
  }
}

TEST(DeflektorishLevelTest, ActionLevelsContainFiniteCrawlerWaves) {
  for (int i = 1; i <= 20; ++i) {
    const Deflektorish::LevelConfig level = Deflektorish::loadLevel(levelPath(i));
    if (i < 4) {
      EXPECT_TRUE(level.enemies.empty()) << level.name;
      continue;
    }
    ASSERT_FALSE(level.enemies.empty()) << level.name;
    if (i <= 10) {
      EXPECT_LE(level.enemies.front().spawnDelay, 2.0f)
          << level.name << " should introduce its first crawler before a fast clear";
    }
    for (const Deflektorish::EnemyConfig &nest : level.enemies) {
      EXPECT_GT(nest.maxSpawns, 0) << level.name;
      EXPECT_GT(nest.spawnInterval, 0.0f) << level.name;
      EXPECT_GT(nest.destroySeconds, 0.0f) << level.name;
    }
  }
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

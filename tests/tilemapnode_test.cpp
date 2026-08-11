#include "tilemapnode.h"

#include <gtest/gtest.h>

namespace {

DL::TileMapConfig testConfig() {
  DL::TileMapConfig config;
  config.mapWidth = 4;
  config.mapHeight = 2;
  config.tileWorldSize = 2.0f;
  config.layers.push_back({
      .name = "Ground",
      .tiles = {{.tileIndex = 3, .x = 1, .y = 0, .flipX = true}},
  });
  return config;
}

TEST(TileMapNodeTest, GetsSetsReplacesAndClearsSparseCells) {
  TileMapNode node(testConfig(), nullptr);

  const auto initial = node.tile("Ground", {1, 0});
  ASSERT_TRUE(initial.has_value());
  EXPECT_EQ(initial->tileIndex, 3u);
  EXPECT_TRUE(initial->flipX);

  EXPECT_TRUE(node.setTile("Ground", {2, 1}, {.tileIndex = 10, .flipY = true}));
  auto changed = node.tile("Ground", {2, 1});
  ASSERT_TRUE(changed.has_value());
  EXPECT_EQ(changed->tileIndex, 10u);
  EXPECT_TRUE(changed->flipY);

  EXPECT_TRUE(node.setTile("Ground", {2, 1}, {.tileIndex = 4}));
  changed = node.tile("Ground", {2, 1});
  ASSERT_TRUE(changed.has_value());
  EXPECT_EQ(changed->tileIndex, 4u);
  EXPECT_FALSE(changed->flipY);

  EXPECT_TRUE(node.clearTile("Ground", {2, 1}));
  EXPECT_FALSE(node.tile("Ground", {2, 1}).has_value());
  EXPECT_FALSE(node.clearTile("Ground", {2, 1}));
}

TEST(TileMapNodeTest, RejectsUnknownLayersAndOutOfBoundsCells) {
  TileMapNode node(testConfig(), nullptr);

  EXPECT_FALSE(node.setTile("Missing", {0, 0}, {.tileIndex = 1}));
  EXPECT_FALSE(node.setTile("Ground", {-1, 0}, {.tileIndex = 1}));
  EXPECT_FALSE(node.setTile("Ground", {4, 0}, {.tileIndex = 1}));
  EXPECT_FALSE(node.clearTile("Ground", {0, 2}));
  EXPECT_FALSE(node.tile("Ground", {-1, 0}).has_value());
}

TEST(TileMapNodeTest, ConvertsBetweenMapCellsAndCenteredLocalPositions) {
  TileMapNode node(testConfig(), nullptr);

  EXPECT_EQ(node.mapToLocal({0, 0}), glm::vec2(-3.0f, 1.0f));
  EXPECT_EQ(node.mapToLocal({3, 1}), glm::vec2(3.0f, -1.0f));
  EXPECT_EQ(node.localToMap({-3.0f, 1.0f}), glm::ivec2(0, 0));
  EXPECT_EQ(node.localToMap({3.0f, -1.0f}), glm::ivec2(3, 1));
  EXPECT_EQ(node.localToMap({0.0f, 0.0f}), glm::ivec2(2, 1));
  EXPECT_FALSE(node.localToMap({4.0f, 0.0f}).has_value());
  EXPECT_FALSE(node.localToMap({0.0f, -2.0f}).has_value());
}

} // namespace

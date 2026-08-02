#include "meshasset.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <glm/geometric.hpp>

namespace {

TEST(MeshAssetTest, LoadsStarterCharacterGlbAsset) {
  const auto asset = DL::loadGltfMeshAsset("../Resources/character-l.glb");

  ASSERT_FALSE(asset.submeshes.empty());
  EXPECT_FALSE(asset.nodes.empty());
  EXPECT_FALSE(asset.animations.empty());
  EXPECT_FALSE(asset.submeshes[0].positions.empty());
  EXPECT_FALSE(asset.submeshes[0].indices.empty());
  ASSERT_FALSE(asset.submeshes[0].texturePath.empty());
  EXPECT_TRUE(asset.submeshes[0].texturePath.ends_with(
      "Resources/Textures/texture-l.png"));
  EXPECT_TRUE(std::filesystem::exists(asset.submeshes[0].texturePath));
}

TEST(MeshAssetTest, DispatchesGltfThroughGenericMeshLoader) {
  const auto asset = DL::loadMeshAsset("../Resources/character-l.glb");

  ASSERT_FALSE(asset.submeshes.empty());
  EXPECT_FALSE(asset.submeshes[0].indices.empty());
}

TEST(MeshAssetTest, LoadsKebnekaiseTerrainAndRouteAssets) {
  const auto terrain = DL::loadMeshAsset(
      "../Resources/Terrain/Kebnekaise/kebnekaise_imported_terrain.gltf");
  const auto route = DL::loadMeshAsset(
      "../Resources/Terrain/Kebnekaise/kebnekaise_imported_route.gltf");

  ASSERT_EQ(terrain.submeshes.size(), 1u);
  EXPECT_EQ(terrain.submeshes[0].positions.size(), 148225u);
  EXPECT_EQ(terrain.submeshes[0].normals.size(), 148225u);
  EXPECT_EQ(terrain.submeshes[0].indices.size(), 884736u);
  ASSERT_FALSE(terrain.submeshes[0].texturePath.empty());
  EXPECT_TRUE(std::filesystem::exists(terrain.submeshes[0].texturePath));
  ASSERT_EQ(route.submeshes.size(), 1u);
  EXPECT_EQ(route.submeshes[0].positions.size(),
            route.submeshes[0].normals.size());
  EXPECT_GT(route.submeshes[0].indices.size(), 1000u);
}

TEST(MeshAssetTest, LoadsKenneyPlatformerGlbAsset) {
  const auto asset = DL::loadMeshAsset(
      "../Resources/Kenney/PlatformerKit/Models/block-grass-large.glb");

  ASSERT_FALSE(asset.submeshes.empty());
  EXPECT_FALSE(asset.nodes.empty());
  EXPECT_FALSE(asset.submeshes[0].positions.empty());
  EXPECT_FALSE(asset.submeshes[0].indices.empty());
  ASSERT_FALSE(asset.submeshes[0].texturePath.empty());
  EXPECT_TRUE(asset.submeshes[0].texturePath.ends_with(
      "Resources/Kenney/PlatformerKit/Models/Textures/colormap.png"));
  EXPECT_TRUE(std::filesystem::exists(asset.submeshes[0].texturePath));
}

TEST(MeshAssetTest, LoadsKenneyPlatformerCharacterGlbAsset) {
  const auto asset = DL::loadMeshAsset(
      "../Resources/Kenney/PlatformerKit/Models/character-oopi.glb");

  ASSERT_FALSE(asset.submeshes.empty());
  EXPECT_FALSE(asset.nodes.empty());
  EXPECT_FALSE(asset.submeshes[0].positions.empty());
  EXPECT_FALSE(asset.submeshes[0].indices.empty());
  ASSERT_FALSE(asset.submeshes[0].texturePath.empty());
  EXPECT_TRUE(std::filesystem::exists(asset.submeshes[0].texturePath));
}

TEST(MeshAssetTest, LoadsStarterCharacterAnimationData) {
  const auto asset = DL::loadGltfMeshAsset("../Resources/character-l.glb");

  ASSERT_FALSE(asset.submeshes.empty());
  ASSERT_FALSE(asset.animations.empty());
  EXPECT_FALSE(asset.animations[0].channels.empty());
  EXPECT_FALSE(asset.animations[0].samplers.empty());
  EXPECT_GT(asset.animations[0].duration, 0.0f);
  EXPECT_FALSE(asset.submeshes[0].positions.empty());
  EXPECT_FALSE(asset.submeshes[0].indices.empty());

  auto pose = DL::makeAnimationPose(asset.nodes.size());
  DL::evaluateAnimationClip(asset.animations[0], 0.5f, pose);
  EXPECT_EQ(pose.nodes.size(), asset.nodes.size());

  bool foundAnimatedRotation = false;
  for (const auto &node : pose.nodes) {
    if (node.hasRotation) {
      foundAnimatedRotation = true;
      EXPECT_NEAR(glm::length(node.rotation), 1.0f, 1e-4f);
      break;
    }
  }
  EXPECT_TRUE(foundAnimatedRotation);
}

} // namespace

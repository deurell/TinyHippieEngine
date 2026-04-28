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

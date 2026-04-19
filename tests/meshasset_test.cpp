#include "meshasset.h"
#include <gtest/gtest.h>
#include <glm/geometric.hpp>

namespace {

TEST(MeshAssetTest, LoadsBundledGltfTriangleAsset) {
  const auto asset = DL::loadGltfMeshAsset("../Resources/triangle.gltf");

  ASSERT_EQ(asset.submeshes.size(), 1u);
  EXPECT_EQ(asset.submeshes[0].positions.size(), 3u);
  EXPECT_EQ(asset.submeshes[0].normals.size(), 3u);
  EXPECT_EQ(asset.submeshes[0].uvs.size(), 3u);
  EXPECT_EQ(asset.submeshes[0].indices.size(), 3u);
}

TEST(MeshAssetTest, DispatchesGltfThroughGenericMeshLoader) {
  const auto asset = DL::loadMeshAsset("../Resources/triangle.gltf");

  ASSERT_EQ(asset.submeshes.size(), 1u);
  EXPECT_EQ(asset.submeshes[0].indices[0], 0u);
  EXPECT_EQ(asset.submeshes[0].indices[1], 1u);
  EXPECT_EQ(asset.submeshes[0].indices[2], 2u);
}

TEST(MeshAssetTest, LoadsBrainStemGlbAsset) {
  const auto asset = DL::loadGltfMeshAsset("../Resources/BrainStem.glb");

  ASSERT_FALSE(asset.submeshes.empty());
  EXPECT_EQ(asset.nodes.size(), 22u);
  ASSERT_EQ(asset.skins.size(), 1u);
  ASSERT_EQ(asset.animations.size(), 1u);
  EXPECT_EQ(asset.skins[0].jointNodeIndices.size(), 18u);
  EXPECT_EQ(asset.skins[0].inverseBindMatrices.size(), 18u);
  EXPECT_EQ(asset.animations[0].channels.size(), 57u);
  EXPECT_EQ(asset.animations[0].samplers.size(), 57u);
  EXPECT_NEAR(asset.animations[0].duration, 34.88f, 0.01f);
  EXPECT_FALSE(asset.submeshes[0].positions.empty());
  EXPECT_FALSE(asset.submeshes[0].indices.empty());

  bool foundSkinningData = false;
  for (const auto &submesh : asset.submeshes) {
    if (!submesh.jointIndices.empty()) {
      foundSkinningData = true;
      EXPECT_EQ(submesh.jointIndices.size(), submesh.positions.size());
      EXPECT_EQ(submesh.jointWeights.size(), submesh.positions.size());
      EXPECT_GE(submesh.skinIndex, 0);
    }
  }
  EXPECT_TRUE(foundSkinningData);

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

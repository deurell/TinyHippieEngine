#include "meshasset.h"
#include <gtest/gtest.h>

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
  EXPECT_FALSE(asset.submeshes[0].positions.empty());
  EXPECT_FALSE(asset.submeshes[0].indices.empty());
}

} // namespace

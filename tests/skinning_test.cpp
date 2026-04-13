#include "animationclip.h"
#include "meshasset.h"
#include "skinning.h"
#include <gtest/gtest.h>
#include <cmath>

namespace {

TEST(SkinningTest, EvaluatesAnimatedWorldTransforms) {
  DL::MeshAsset asset;
  asset.nodes.resize(2);
  asset.nodes[0].baseTranslation = {1.0f, 0.0f, 0.0f};
  asset.nodes[0].parentIndex = -1;
  asset.nodes[1].baseTranslation = {0.0f, 2.0f, 0.0f};
  asset.nodes[1].parentIndex = 0;

  auto pose = DL::makeAnimationPose(asset.nodes.size());
  pose.nodes[0].hasTranslation = true;
  pose.nodes[0].translation = {3.0f, 0.0f, 0.0f};

  const auto worldTransforms = DL::evaluateNodeWorldTransforms(asset, pose);

  ASSERT_EQ(worldTransforms.size(), 2u);
  EXPECT_FLOAT_EQ(worldTransforms[0][3].x, 3.0f);
  EXPECT_FLOAT_EQ(worldTransforms[1][3].x, 3.0f);
  EXPECT_FLOAT_EQ(worldTransforms[1][3].y, 2.0f);
}

TEST(SkinningTest, EvaluatesWorldTransformsWhenParentAppearsAfterChild) {
  DL::MeshAsset asset;
  asset.nodes.resize(2);
  asset.nodes[0].parentIndex = 1;
  asset.nodes[0].baseTranslation = {0.0f, 2.0f, 0.0f};
  asset.nodes[1].parentIndex = -1;
  asset.nodes[1].baseTranslation = {3.0f, 0.0f, 0.0f};

  const auto pose = DL::makeAnimationPose(asset.nodes.size());
  const auto worldTransforms = DL::evaluateNodeWorldTransforms(asset, pose);

  ASSERT_EQ(worldTransforms.size(), 2u);
  EXPECT_FLOAT_EQ(worldTransforms[0][3].x, 3.0f);
  EXPECT_FLOAT_EQ(worldTransforms[0][3].y, 2.0f);
  EXPECT_FLOAT_EQ(worldTransforms[1][3].x, 3.0f);
}

TEST(SkinningTest, ComputesBrainStemSkinMatrices) {
  const auto asset = DL::loadGltfMeshAsset("../Resources/BrainStem.glb");
  ASSERT_EQ(asset.skins.size(), 1u);
  ASSERT_EQ(asset.animations.size(), 1u);

  auto pose = DL::makeAnimationPose(asset.nodes.size());
  DL::evaluateAnimationClip(asset.animations[0], 0.5f, pose);
  const auto skinMatrices = DL::computeSkinMatrices(asset, 0, pose);

  ASSERT_EQ(skinMatrices.size(), 18u);
  bool foundNonIdentity = false;
  for (const auto &matrix : skinMatrices) {
    for (int column = 0; column < 4; ++column) {
      for (int row = 0; row < 4; ++row) {
        EXPECT_TRUE(std::isfinite(matrix[column][row]));
      }
    }
    if (std::abs(matrix[0][0] - 1.0f) > 1e-4f ||
        std::abs(matrix[1][1] - 1.0f) > 1e-4f ||
        std::abs(matrix[2][2] - 1.0f) > 1e-4f ||
        std::abs(matrix[3][0]) > 1e-4f || std::abs(matrix[3][1]) > 1e-4f ||
        std::abs(matrix[3][2]) > 1e-4f) {
      foundNonIdentity = true;
    }
  }
  EXPECT_TRUE(foundNonIdentity);
}

} // namespace

#include "meshassetcache.h"
#include <gtest/gtest.h>

namespace {

TEST(MeshAssetCacheTest, ReusesLoadedMeshAssetInstanceForSamePath) {
  DL::MeshAssetCache cache;

  const auto first = cache.load("../Resources/triangle.gltf");
  const auto second = cache.load("../Resources/triangle.gltf");

  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(first.get(), second.get());
}

} // namespace

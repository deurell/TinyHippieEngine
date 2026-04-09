#include "camera.h"
#include <gtest/gtest.h>
#include <cmath>

namespace {

TEST(CameraTest, PerspectiveTransformFallsBackWhenScreenSizeIsZero) {
  DL::Camera camera({0.0f, 0.0f, 5.0f});

  const glm::mat4 projection = camera.getPerspectiveTransform();

  for (int column = 0; column < 4; ++column) {
    for (int row = 0; row < 4; ++row) {
      EXPECT_TRUE(std::isfinite(projection[column][row]));
    }
  }
}

TEST(CameraTest, PerspectiveTransformUsesConfiguredAspectRatio) {
  DL::Camera camera({0.0f, 0.0f, 5.0f});
  camera.mScreenSize = {1920.0f, 1080.0f};

  const glm::mat4 projection = camera.getPerspectiveTransform();
  const float expected =
      1.0f / (std::tan(glm::radians(camera.mFov) * 0.5f) *
              (camera.mScreenSize.x / camera.mScreenSize.y));

  EXPECT_NEAR(projection[0][0], expected, 1e-5f);
}

} // namespace

#include "cameranode.h"

#include <cmath>
#include <gtest/gtest.h>

TEST(CameraNodeTest, LookAtWorldUpdatesNodeRotationAndCameraOrientation) {
  DL::CameraNode cameraNode;
  cameraNode.setLocalPosition({0.0f, 0.0f, 0.0f});

  cameraNode.lookAtWorld({1.0f, 0.0f, 0.0f});
  cameraNode.update({});

  EXPECT_NEAR(cameraNode.camera().getPosition().x, 0.0f, 1e-5f);
  EXPECT_NEAR(cameraNode.camera().getPosition().y, 0.0f, 1e-5f);
  EXPECT_NEAR(cameraNode.camera().getPosition().z, 0.0f, 1e-5f);

  DL::Camera expected({0.0f, 0.0f, 0.0f});
  expected.lookAt({1.0f, 0.0f, 0.0f});
  EXPECT_NEAR(std::abs(glm::dot(cameraNode.camera().mOrientation,
                                expected.mOrientation)),
              1.0f, 1e-5f);
}

TEST(CameraNodeTest, TranslateLocalMovesAuthoredNodeTransform) {
  DL::CameraNode cameraNode;
  cameraNode.setLocalPosition({0.0f, 0.0f, 0.0f});
  cameraNode.lookAtWorld({1.0f, 0.0f, 0.0f});

  cameraNode.translateLocal({0.0f, 0.0f, -2.0f});
  cameraNode.update({});

  EXPECT_NEAR(cameraNode.getLocalPosition().x, 2.0f, 1e-5f);
  EXPECT_NEAR(cameraNode.getLocalPosition().y, 0.0f, 1e-5f);
  EXPECT_NEAR(cameraNode.getLocalPosition().z, 0.0f, 1e-5f);
  EXPECT_NEAR(cameraNode.camera().getPosition().x, 2.0f, 1e-5f);
  EXPECT_NEAR(cameraNode.camera().getPosition().y, 0.0f, 1e-5f);
  EXPECT_NEAR(cameraNode.camera().getPosition().z, 0.0f, 1e-5f);
}

TEST(CameraNodeTest, ProjectionSettingsUpdateCamera) {
  DL::CameraNode cameraNode;

  cameraNode.setProjection(DL::CameraProjection::Orthographic);
  cameraNode.setOrthographicHeight(7.5f);

  EXPECT_EQ(cameraNode.projection(), DL::CameraProjection::Orthographic);
  EXPECT_FLOAT_EQ(cameraNode.orthographicHeight(), 7.5f);
  EXPECT_EQ(cameraNode.camera().mProjection,
            DL::CameraProjection::Orthographic);
  EXPECT_FLOAT_EQ(cameraNode.camera().mOrthographicHeight, 7.5f);
}

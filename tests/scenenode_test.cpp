#include "scenenode.h"
#include <gtest/gtest.h>
#include <glm/gtx/quaternion.hpp>

namespace {

class TestSceneNode final : public DL::SceneNode {
public:
  using DL::SceneNode::SceneNode;
};

TEST(SceneNodeTest, LocalTransformBecomesWorldTransformForRootNode) {
  TestSceneNode node;
  node.setLocalPosition({1.0f, 2.0f, 3.0f});
  node.setLocalScale({2.0f, 3.0f, 4.0f});

  node.update({});

  EXPECT_NEAR(node.getWorldPosition().x, 1.0f, 1e-6f);
  EXPECT_NEAR(node.getWorldPosition().y, 2.0f, 1e-6f);
  EXPECT_NEAR(node.getWorldPosition().z, 3.0f, 1e-6f);
  EXPECT_NEAR(node.getWorldScale().x, 2.0f, 1e-6f);
  EXPECT_NEAR(node.getWorldScale().y, 3.0f, 1e-6f);
  EXPECT_NEAR(node.getWorldScale().z, 4.0f, 1e-6f);
}

TEST(SceneNodeTest, ChildWorldPositionIncludesParentTransform) {
  auto child = std::make_unique<TestSceneNode>();
  auto *childPtr = child.get();

  TestSceneNode parent;
  parent.setLocalPosition({10.0f, 0.0f, 0.0f});
  childPtr->setLocalPosition({0.0f, 5.0f, 0.0f});
  parent.addChild(std::move(child));

  parent.update({});

  EXPECT_NEAR(childPtr->getWorldPosition().x, 10.0f, 1e-6f);
  EXPECT_NEAR(childPtr->getWorldPosition().y, 5.0f, 1e-6f);
  EXPECT_NEAR(childPtr->getWorldPosition().z, 0.0f, 1e-6f);
}

TEST(SceneNodeTest, MarkDirtyPropagatesToChildrenAfterParentChanges) {
  auto child = std::make_unique<TestSceneNode>();
  auto *childPtr = child.get();

  TestSceneNode parent;
  childPtr->setLocalPosition({1.0f, 0.0f, 0.0f});
  parent.addChild(std::move(child));

  parent.update({});
  EXPECT_NEAR(childPtr->getWorldPosition().x, 1.0f, 1e-6f);

  parent.setLocalPosition({2.0f, 0.0f, 0.0f});
  parent.update({});

  EXPECT_NEAR(childPtr->getWorldPosition().x, 3.0f, 1e-6f);
}

TEST(SceneNodeTest, WorldRotationTracksLocalRotation) {
  TestSceneNode node;
  const glm::quat rotation = glm::angleAxis(glm::half_pi<float>(),
                                            glm::vec3(0.0f, 0.0f, 1.0f));
  node.setLocalRotation(rotation);

  node.update({});

  const glm::quat worldRotation = node.getWorldRotation();
  EXPECT_NEAR(glm::angle(worldRotation), glm::angle(rotation), 1e-5f);
  EXPECT_NEAR(glm::axis(worldRotation).z, glm::axis(rotation).z, 1e-5f);
}

} // namespace

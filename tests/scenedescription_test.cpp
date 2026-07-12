#include "scenedescription.h"

#include "meshnode.h"
#include <gtest/gtest.h>

namespace {

constexpr char kSceneSource[] = R"json(
{
  "name": "unit_scene",
  "nodes": [
    {
      "name": "root",
      "type": "SceneNode",
      "transform": {
        "position": [1.0, 2.0, 3.0],
        "rotationEuler": [0.0, 90.0, 0.0],
        "scale": [2.0, 2.0, 2.0]
      },
      "children": [
        {
          "name": "mesh_child",
          "type": "MeshNode",
          "mesh": "Resources/character-l.glb",
          "transform": {
            "position": [0.5, 0.0, 0.0],
            "scale": [0.5, 0.5, 0.5]
          },
          "animation": {
            "clip": "idle",
            "playing": false,
            "looping": true,
            "playbackSpeed": 0.75
          },
          "visualizer": {
            "ambientStrength": 0.6,
            "specularStrength": 0.1
          }
        }
      ]
    }
  ]
}
)json";

} // namespace

TEST(SceneDescriptionTest, ParsesNodeHierarchyAndMeshSettings) {
  const DL::SceneDescription scene =
      DL::parseSceneDescription(kSceneSource, "unit");

  ASSERT_EQ(scene.name, "unit_scene");
  ASSERT_EQ(scene.nodes.size(), 1u);
  const DL::SceneNodeDescription &root = scene.nodes[0];
  EXPECT_EQ(root.name, "root");
  EXPECT_EQ(root.type, "SceneNode");
  EXPECT_EQ(root.position, glm::vec3(1.0f, 2.0f, 3.0f));
  EXPECT_EQ(root.scale, glm::vec3(2.0f, 2.0f, 2.0f));
  ASSERT_EQ(root.children.size(), 1u);

  const DL::SceneNodeDescription &child = root.children[0];
  EXPECT_EQ(child.name, "mesh_child");
  EXPECT_EQ(child.type, "MeshNode");
  EXPECT_EQ(child.mesh, "Resources/character-l.glb");
  ASSERT_TRUE(child.animation.has_value());
  EXPECT_EQ(child.animation->clip, "idle");
  EXPECT_FALSE(child.animation->playing);
  EXPECT_FLOAT_EQ(child.animation->playbackSpeed, 0.75f);
  EXPECT_FLOAT_EQ(child.visualizerSettings.ambientStrength, 0.6f);
  EXPECT_FLOAT_EQ(child.visualizerSettings.specularStrength, 0.1f);
}

TEST(SceneDescriptionTest, BuildsRuntimeNodeTree) {
  const DL::SceneDescription scene =
      DL::parseSceneDescription(kSceneSource, "unit");

  std::unique_ptr<DL::SceneNode> root =
      DL::buildSceneNode(scene.nodes[0], DL::SceneBuilderContext{});

  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->getDebugName(), "root");
  EXPECT_EQ(root->getLocalPosition(), glm::vec3(1.0f, 2.0f, 3.0f));
  ASSERT_EQ(root->children.size(), 1u);

  DL::SceneNode *child = DL::findSceneNodeByName(*root, "mesh_child");
  ASSERT_NE(child, nullptr);
  EXPECT_NE(dynamic_cast<MeshNode *>(child), nullptr);
  EXPECT_EQ(child->getLocalPosition(), glm::vec3(0.5f, 0.0f, 0.0f));
  EXPECT_EQ(child->getLocalScale(), glm::vec3(0.5f, 0.5f, 0.5f));
}

TEST(SceneDescriptionTest, RejectsUnknownNodeTypes) {
  DL::SceneNodeDescription description;
  description.type = "MissingNode";

  EXPECT_THROW(DL::buildSceneNode(description, DL::SceneBuilderContext{}),
               std::runtime_error);
}

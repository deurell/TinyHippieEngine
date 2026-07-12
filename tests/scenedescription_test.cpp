#include "scenedescription.h"

#include "meshnode.h"
#include <algorithm>
#include <filesystem>
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
          "name": "camera_child",
          "type": "CameraNode",
          "active": true,
          "fov": 38.0,
          "lookAt": [0.0, 0.5, 0.0]
        },
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
        },
        {
          "name": "plane_child",
          "type": "PlaneNode",
          "plane": "Spinner",
          "color": [0.1, 0.2, 0.3, 0.4]
        },
        {
          "name": "shape_child",
          "type": "PhongShapeNode",
          "shape": "Cylinder",
          "material": {
            "diffuse": [0.4, 0.5, 0.6],
            "ambient": [0.1, 0.1, 0.2],
            "specular": [0.7, 0.8, 0.9],
            "shininess": 48.0
          }
        },
        {
          "name": "sprite_child",
          "type": "SpriteNode",
          "image": "Resources/Textures/texture-l.png",
          "billboard": true
        },
        {
          "name": "text_child",
          "type": "TextNode",
          "text": "hello",
          "alignment": "Center",
          "anchor": "BottomCenter",
          "fontSize": 56.0,
          "textColor": [1.0, 0.95, 0.82, 1.0],
          "shadowColor": [0.0, 0.0, 0.0, 0.65],
          "shadowOffset": [2.0, -2.0],
          "billboard": true
        },
        {
          "name": "particles_child",
          "type": "ParticleSystemNode",
          "particle": "WaterFountain",
          "billboard": true
        }
      ]
    }
  ]
}
)json";

constexpr char kBuildSceneSource[] = R"json(
{
  "name": "build_scene",
  "nodes": [
    {
      "name": "root",
      "type": "SceneNode",
      "transform": {
        "position": [1.0, 2.0, 3.0],
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
          }
        }
      ]
    }
  ]
}
)json";

constexpr char kPlaneSceneSource[] = R"json(
{
  "name": "plane_scene",
  "nodes": [
    {
      "name": "ground_plane",
      "type": "PlaneNode",
      "plane": "Simple",
      "transform": {
        "position": [1.0, -0.04, 2.0],
        "rotationEuler": [-90.0, 0.0, 0.0],
        "scale": [8.0, 5.0, 1.0]
      }
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
  ASSERT_EQ(root.children.size(), 7u);

  EXPECT_EQ(root.children[0].type, "CameraNode");
  EXPECT_TRUE(root.children[0].active);
  EXPECT_FLOAT_EQ(root.children[0].fov, 38.0f);
  ASSERT_TRUE(root.children[0].lookAt.has_value());
  EXPECT_EQ(*root.children[0].lookAt, glm::vec3(0.0f, 0.5f, 0.0f));

  const DL::SceneNodeDescription &child = root.children[1];
  EXPECT_EQ(child.name, "mesh_child");
  EXPECT_EQ(child.type, "MeshNode");
  EXPECT_EQ(child.mesh, "Resources/character-l.glb");
  ASSERT_TRUE(child.animation.has_value());
  EXPECT_EQ(child.animation->clip, "idle");
  EXPECT_FALSE(child.animation->playing);
  EXPECT_FLOAT_EQ(child.animation->playbackSpeed, 0.75f);
  EXPECT_FLOAT_EQ(child.visualizerSettings.ambientStrength, 0.6f);
  EXPECT_FLOAT_EQ(child.visualizerSettings.specularStrength, 0.1f);

  EXPECT_EQ(root.children[2].type, "PlaneNode");
  EXPECT_EQ(root.children[2].plane, "Spinner");
  EXPECT_EQ(root.children[2].color, glm::vec4(0.1f, 0.2f, 0.3f, 0.4f));
  EXPECT_EQ(root.children[3].type, "PhongShapeNode");
  EXPECT_EQ(root.children[3].shape, "Cylinder");
  EXPECT_EQ(root.children[3].material.diffuse, glm::vec3(0.4f, 0.5f, 0.6f));
  EXPECT_FLOAT_EQ(root.children[3].material.shininess, 48.0f);
  EXPECT_EQ(root.children[4].image, "Resources/Textures/texture-l.png");
  EXPECT_TRUE(root.children[4].billboard);
  EXPECT_EQ(root.children[5].text, "hello");
  EXPECT_EQ(root.children[5].textAlignment, "Center");
  EXPECT_EQ(root.children[5].textAnchor, "BottomCenter");
  EXPECT_FLOAT_EQ(root.children[5].fontSize, 56.0f);
  EXPECT_EQ(root.children[5].textColor, glm::vec4(1.0f, 0.95f, 0.82f, 1.0f));
  EXPECT_EQ(root.children[5].shadowColor, glm::vec4(0.0f, 0.0f, 0.0f, 0.65f));
  EXPECT_EQ(root.children[5].shadowOffset, glm::vec2(2.0f, -2.0f));
  EXPECT_TRUE(root.children[5].billboard);
  EXPECT_EQ(root.children[6].particle, "WaterFountain");
  EXPECT_TRUE(root.children[6].billboard);
}

TEST(SceneDescriptionTest, BuildsRuntimeNodeTree) {
  const DL::SceneDescription scene =
      DL::parseSceneDescription(kBuildSceneSource, "build_unit");

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

TEST(SceneDescriptionTest, PlaneNodeKeepsAuthoredTransformAfterInit) {
  const DL::SceneDescription scene =
      DL::parseSceneDescription(kPlaneSceneSource, "plane_unit");

  std::unique_ptr<DL::SceneNode> root =
      DL::buildSceneNode(scene.nodes[0], DL::SceneBuilderContext{});

  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->getDebugName(), "ground_plane");
  EXPECT_EQ(root->getLocalPosition(), glm::vec3(1.0f, -0.04f, 2.0f));
  EXPECT_EQ(root->getLocalScale(), glm::vec3(8.0f, 5.0f, 1.0f));
}

TEST(SceneDescriptionTest, DefaultFactoryRegistersEngineNodeTypes) {
  const DL::SceneNodeFactory factory = DL::createDefaultSceneNodeFactory();

  EXPECT_TRUE(factory.hasNodeType("SceneNode"));
  EXPECT_TRUE(factory.hasNodeType("CameraNode"));
  EXPECT_TRUE(factory.hasNodeType("MeshNode"));
  EXPECT_TRUE(factory.hasNodeType("SpriteNode"));
  EXPECT_TRUE(factory.hasNodeType("TextNode"));
  EXPECT_TRUE(factory.hasNodeType("PlaneNode"));
  EXPECT_TRUE(factory.hasNodeType("PhongShapeNode"));
  EXPECT_TRUE(factory.hasNodeType("ParticleSystemNode"));
  EXPECT_FALSE(factory.hasNodeType("MissingNode"));
}

TEST(SceneDescriptionTest, LoadsStarterSceneFile) {
  std::filesystem::path path = "../Resources/Scenes/simple_starter.scene.json";
  if (!std::filesystem::exists(path)) {
    path = "Resources/Scenes/simple_starter.scene.json";
  }

  const DL::SceneDescription scene = DL::loadSceneDescription(path);

  EXPECT_EQ(scene.name, "tiny_hippie_sample_scene");
  ASSERT_GE(scene.nodes.size(), 10u);
  EXPECT_NE(std::ranges::find_if(scene.nodes,
                                 [](const DL::SceneNodeDescription &node) {
                                   return node.name == "main_camera" &&
                                          node.type == "CameraNode" &&
                                          node.active;
                                 }),
            scene.nodes.end());
  EXPECT_NE(std::ranges::find_if(scene.nodes,
                                 [](const DL::SceneNodeDescription &node) {
                                   return node.name == "hero" &&
                                          node.type == "MeshNode";
                                 }),
            scene.nodes.end());
  EXPECT_NE(std::ranges::find_if(scene.nodes,
                                 [](const DL::SceneNodeDescription &node) {
                                   return node.type == "ParticleSystemNode";
                                 }),
            scene.nodes.end());
}

TEST(SceneDescriptionTest, RejectsUnknownNodeTypes) {
  DL::SceneNodeDescription description;
  description.type = "MissingNode";

  EXPECT_THROW(DL::buildSceneNode(description, DL::SceneBuilderContext{}),
               std::runtime_error);
}

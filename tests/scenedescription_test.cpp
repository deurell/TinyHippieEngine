#include "scenedescription.h"

#include "meshnode.h"
#include "spriteanimationnode.h"
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
          "projection": "Orthographic",
          "orthographicHeight": 7.25,
          "lookAt": [0.0, 0.5, 0.0]
        },
        {
          "name": "light_child",
          "type": "LightNode",
          "light": {
            "kind": "Directional",
            "direction": [0.2, 1.0, 0.3],
            "color": [1.0, 0.9, 0.7],
            "intensity": 1.4,
            "ambientStrength": 0.35
          }
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
          "sourceRect": [16.0, 32.0, 16.0, 16.0],
          "flipX": true,
          "flipDiagonal": true,
          "billboard": true
        },
        {
          "name": "sprite_batch_child",
          "type": "SpriteBatchNode",
          "spriteBatch": {
            "image": "Resources/Kenney/TinyDungeon/Tilemap/tilemap_packed.png",
            "sprites": [
              {
                "position": [1.0, 2.0, 0.1],
                "size": [0.5, 0.75],
                "rotationDegrees": 15.0,
                "sourceRect": [16.0, 32.0, 16.0, 16.0],
                "flipY": true
              },
              {
                "position": [-1.0, 0.0, 0.2],
                "size": [1.0, 1.0],
                "sourceRect": [32.0, 48.0, 16.0, 16.0],
                "flipDiagonal": true
              }
            ]
          }
        },
        {
          "name": "sprite_animation_child",
          "type": "SpriteAnimationNode",
          "image": "Resources/Kenney/TinyDungeon/Tilemap/tilemap_packed.png",
          "billboard": true,
          "spriteAnimation": {
            "fps": 6.0,
            "playing": true,
            "looping": false,
            "frames": [
              [0.0, 32.0, 16.0, 16.0],
              [16.0, 32.0, 16.0, 16.0],
              [32.0, 32.0, 16.0, 16.0]
            ]
          }
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

const DL::SceneNodeDescription *
findDescriptionByName(const DL::SceneNodeDescription &node,
                      std::string_view name) {
  if (node.name == name) {
    return &node;
  }
  for (const auto &child : node.children) {
    if (const auto *found = findDescriptionByName(child, name)) {
      return found;
    }
  }
  return nullptr;
}

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
  ASSERT_EQ(root.children.size(), 10u);

  EXPECT_EQ(root.children[0].type, "CameraNode");
  EXPECT_TRUE(root.children[0].active);
  EXPECT_FLOAT_EQ(root.children[0].fov, 38.0f);
  EXPECT_EQ(root.children[0].projection, "Orthographic");
  EXPECT_FLOAT_EQ(root.children[0].orthographicHeight, 7.25f);
  ASSERT_TRUE(root.children[0].lookAt.has_value());
  EXPECT_EQ(*root.children[0].lookAt, glm::vec3(0.0f, 0.5f, 0.0f));

  EXPECT_EQ(root.children[1].type, "LightNode");
  EXPECT_EQ(root.children[1].light.kind, "Directional");
  ASSERT_TRUE(root.children[1].light.direction.has_value());
  EXPECT_EQ(*root.children[1].light.direction, glm::vec3(0.2f, 1.0f, 0.3f));
  EXPECT_EQ(root.children[1].light.color, glm::vec3(1.0f, 0.9f, 0.7f));
  EXPECT_FLOAT_EQ(root.children[1].light.intensity, 1.4f);
  EXPECT_FLOAT_EQ(root.children[1].light.ambientStrength, 0.35f);

  const DL::SceneNodeDescription &child = root.children[2];
  EXPECT_EQ(child.name, "mesh_child");
  EXPECT_EQ(child.type, "MeshNode");
  EXPECT_EQ(child.mesh, "Resources/character-l.glb");
  ASSERT_TRUE(child.animation.has_value());
  EXPECT_EQ(child.animation->clip, "idle");
  EXPECT_FALSE(child.animation->playing);
  EXPECT_FLOAT_EQ(child.animation->playbackSpeed, 0.75f);
  EXPECT_FLOAT_EQ(child.visualizerSettings.ambientStrength, 0.6f);
  EXPECT_FLOAT_EQ(child.visualizerSettings.specularStrength, 0.1f);

  EXPECT_EQ(root.children[3].type, "PlaneNode");
  EXPECT_EQ(root.children[3].plane, "Spinner");
  EXPECT_EQ(root.children[3].color, glm::vec4(0.1f, 0.2f, 0.3f, 0.4f));
  EXPECT_EQ(root.children[4].type, "PhongShapeNode");
  EXPECT_EQ(root.children[4].shape, "Cylinder");
  EXPECT_EQ(root.children[4].material.diffuse, glm::vec3(0.4f, 0.5f, 0.6f));
  EXPECT_FLOAT_EQ(root.children[4].material.shininess, 48.0f);
  EXPECT_EQ(root.children[5].image, "Resources/Textures/texture-l.png");
  EXPECT_EQ(root.children[5].sourceRect, glm::vec4(16.0f, 32.0f, 16.0f, 16.0f));
  EXPECT_TRUE(root.children[5].flipX);
  EXPECT_FALSE(root.children[5].flipY);
  EXPECT_TRUE(root.children[5].flipDiagonal);
  EXPECT_TRUE(root.children[5].billboard);
  EXPECT_EQ(root.children[6].type, "SpriteBatchNode");
  EXPECT_EQ(root.children[6].spriteBatch.imagePath,
            "Resources/Kenney/TinyDungeon/Tilemap/tilemap_packed.png");
  ASSERT_EQ(root.children[6].spriteBatch.sprites.size(), 2u);
  EXPECT_EQ(root.children[6].spriteBatch.sprites[0].position,
            glm::vec3(1.0f, 2.0f, 0.1f));
  EXPECT_EQ(root.children[6].spriteBatch.sprites[0].size,
            glm::vec2(0.5f, 0.75f));
  EXPECT_FLOAT_EQ(root.children[6].spriteBatch.sprites[0].rotationDegrees,
                  15.0f);
  EXPECT_EQ(root.children[6].spriteBatch.sprites[0].sourceRectPixels,
            glm::vec4(16.0f, 32.0f, 16.0f, 16.0f));
  EXPECT_TRUE(root.children[6].spriteBatch.sprites[0].flipY);
  EXPECT_TRUE(root.children[6].spriteBatch.sprites[1].flipDiagonal);
  EXPECT_EQ(root.children[7].type, "SpriteAnimationNode");
  EXPECT_EQ(root.children[7].image,
            "Resources/Kenney/TinyDungeon/Tilemap/tilemap_packed.png");
  EXPECT_TRUE(root.children[7].billboard);
  EXPECT_FLOAT_EQ(root.children[7].spriteAnimation.fps, 6.0f);
  EXPECT_TRUE(root.children[7].spriteAnimation.playing);
  EXPECT_FALSE(root.children[7].spriteAnimation.looping);
  ASSERT_EQ(root.children[7].spriteAnimation.frames.size(), 3u);
  EXPECT_EQ(root.children[7].spriteAnimation.frames[1].sourceRect,
            glm::vec4(16.0f, 32.0f, 16.0f, 16.0f));
  EXPECT_EQ(root.children[8].text, "hello");
  EXPECT_EQ(root.children[8].textAlignment, "Center");
  EXPECT_EQ(root.children[8].textAnchor, "BottomCenter");
  EXPECT_FLOAT_EQ(root.children[8].fontSize, 56.0f);
  EXPECT_EQ(root.children[8].textColor, glm::vec4(1.0f, 0.95f, 0.82f, 1.0f));
  EXPECT_EQ(root.children[8].shadowColor, glm::vec4(0.0f, 0.0f, 0.0f, 0.65f));
  EXPECT_EQ(root.children[8].shadowOffset, glm::vec2(2.0f, -2.0f));
  EXPECT_TRUE(root.children[8].billboard);
  EXPECT_EQ(root.children[9].particle, "WaterFountain");
  EXPECT_TRUE(root.children[9].billboard);
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

TEST(SceneDescriptionTest, SpriteAnimationNodeAdvancesAtlasFrames) {
  SpriteAnimationNode node("atlas.png", nullptr, nullptr);
  node.setAnimation(SpriteAnimationConfig{
      .frames = {{.sourceRectPixels = {0.0f, 0.0f, 16.0f, 16.0f}},
                 {.sourceRectPixels = {16.0f, 0.0f, 16.0f, 16.0f}},
                 {.sourceRectPixels = {32.0f, 0.0f, 16.0f, 16.0f}}},
      .fps = 4.0f,
      .playing = true,
      .looping = true});

  EXPECT_EQ(node.currentFrameIndex(), 0u);
  EXPECT_EQ(node.atlasSourceRectPixels(),
            glm::vec4(0.0f, 0.0f, 16.0f, 16.0f));

  node.fixedUpdate(DL::FrameContext{.delta_time = 0.25f});

  EXPECT_EQ(node.currentFrameIndex(), 1u);
  EXPECT_EQ(node.atlasSourceRectPixels(),
            glm::vec4(16.0f, 0.0f, 16.0f, 16.0f));
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
  EXPECT_TRUE(factory.hasNodeType("LightNode"));
  EXPECT_TRUE(factory.hasNodeType("MeshNode"));
  EXPECT_TRUE(factory.hasNodeType("SpriteNode"));
  EXPECT_TRUE(factory.hasNodeType("SpriteAnimationNode"));
  EXPECT_TRUE(factory.hasNodeType("SpriteBatchNode"));
  EXPECT_TRUE(factory.hasNodeType("TextNode"));
  EXPECT_TRUE(factory.hasNodeType("TileMapNode"));
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
  const auto sunLight =
      std::ranges::find_if(scene.nodes, [](const DL::SceneNodeDescription &node) {
        return node.name == "sun_light" && node.type == "LightNode" &&
               node.light.kind == "Directional";
      });
  ASSERT_NE(sunLight, scene.nodes.end());
  EXPECT_EQ(sunLight->position, glm::vec3(0.62f, 1.22f, -2.05f));
  EXPECT_NE(findDescriptionByName(*sunLight, "sun_light_marker"), nullptr);
  EXPECT_NE(findDescriptionByName(*sunLight, "light_sample_label"), nullptr);
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
  EXPECT_NE(std::ranges::find_if(scene.nodes,
                                 [](const DL::SceneNodeDescription &node) {
                                   return node.name == "sprite_badge" &&
                                          node.type == "SpriteNode" &&
                                          node.billboard &&
                                          node.image ==
                                              "Resources/Textures/generated/"
                                              "retro-crystal-terminal.png";
                                 }),
            scene.nodes.end());
  const auto spriteAnimation =
      std::ranges::find_if(scene.nodes, [](const DL::SceneNodeDescription &node) {
        return node.name == "sprite_animation_magic_torch" &&
               node.type == "SpriteAnimationNode";
      });
  ASSERT_NE(spriteAnimation, scene.nodes.end());
  EXPECT_EQ(spriteAnimation->spriteAnimation.frames.size(), 4u);
  EXPECT_FLOAT_EQ(spriteAnimation->spriteAnimation.fps, 2.0f);
  const auto spriteBatch =
      std::ranges::find_if(scene.nodes, [](const DL::SceneNodeDescription &node) {
        return node.name == "sprite_batch_cluster" &&
               node.type == "SpriteBatchNode";
      });
  ASSERT_NE(spriteBatch, scene.nodes.end());
  EXPECT_EQ(spriteBatch->spriteBatch.imagePath,
            "Resources/Kenney/TinyDungeon/Tilemap/tilemap_packed.png");
  EXPECT_EQ(spriteBatch->spriteBatch.sprites.size(), 5u);
  const auto hierarchySample =
      std::ranges::find_if(scene.nodes, [](const DL::SceneNodeDescription &node) {
        return node.name == "hierarchy_parent" && node.type == "SceneNode";
      });
  ASSERT_NE(hierarchySample, scene.nodes.end());
  EXPECT_EQ(hierarchySample->children.size(), 3u);
  EXPECT_NE(std::ranges::find_if(hierarchySample->children,
                                 [](const DL::SceneNodeDescription &node) {
                                   return node.name == "hierarchy_sun" &&
                                          node.type == "PhongShapeNode";
                                 }),
            hierarchySample->children.end());
  EXPECT_NE(std::ranges::find_if(hierarchySample->children,
                                 [](const DL::SceneNodeDescription &node) {
                                   return node.name ==
                                              "hierarchy_planet_orbit" &&
                                          node.type == "SceneNode" &&
                                          node.children.size() == 2u;
                                 }),
            hierarchySample->children.end());
  EXPECT_NE(std::ranges::find_if(hierarchySample->children,
                                 [](const DL::SceneNodeDescription &node) {
                                   return node.name == "hierarchy_stand" &&
                                          node.type == "PhongShapeNode";
                                 }),
            hierarchySample->children.end());
}

TEST(SceneDescriptionTest, LoadsKenneyPlatformerSceneFile) {
  std::filesystem::path path =
      "../Resources/Scenes/kenney_platformer.scene.json";
  if (!std::filesystem::exists(path)) {
    path = "Resources/Scenes/kenney_platformer.scene.json";
  }

  const DL::SceneDescription scene = DL::loadSceneDescription(path);

  EXPECT_EQ(scene.name, "kenney_platformer_glb_spike");
  ASSERT_GE(scene.nodes.size(), 8u);
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
                                          node.type == "MeshNode" &&
                                          node.mesh ==
                                              "Resources/Kenney/PlatformerKit/"
                                              "Models/character-oopi.glb";
                                 }),
            scene.nodes.end());
  const auto platformRoot =
      std::ranges::find_if(scene.nodes, [](const DL::SceneNodeDescription &node) {
        return node.name == "platform_root" && node.type == "SceneNode";
      });
  ASSERT_NE(platformRoot, scene.nodes.end());
  EXPECT_GE(platformRoot->children.size(), 4u);
}

TEST(SceneDescriptionTest, LoadsTinyDungeonAtlasSceneFile) {
  std::filesystem::path path =
      "../Resources/Scenes/tiny_dungeon_atlas.scene.json";
  if (!std::filesystem::exists(path)) {
    path = "Resources/Scenes/tiny_dungeon_atlas.scene.json";
  }

  const DL::SceneDescription scene = DL::loadSceneDescription(path);

  EXPECT_EQ(scene.name, "kenney_tiny_dungeon_atlas_sample");
  ASSERT_EQ(scene.nodes.size(), 3u);
  EXPECT_EQ(scene.nodes[0].name, "main_camera");
  EXPECT_EQ(scene.nodes[0].type, "CameraNode");
  EXPECT_TRUE(scene.nodes[0].active);
  EXPECT_EQ(scene.nodes[0].projection, "Orthographic");
  EXPECT_FLOAT_EQ(scene.nodes[0].orthographicHeight, 6.8f);

  const auto *hero = findDescriptionByName(scene.nodes[1], "hero");
  ASSERT_NE(hero, nullptr);
  EXPECT_EQ(hero->type, "SceneNode");
  const auto *heroSprite = findDescriptionByName(*hero, "hero_sprite");
  ASSERT_NE(heroSprite, nullptr);
  EXPECT_EQ(heroSprite->type, "SpriteNode");
  EXPECT_EQ(heroSprite->sourceRect,
            glm::vec4(48.0f, 128.0f, 16.0f, 16.0f));
  EXPECT_FLOAT_EQ(heroSprite->position.z, 0.35f);
  EXPECT_EQ(heroSprite->scale, glm::vec3(0.17f));

  const DL::SceneNodeDescription &tileMap = scene.nodes[2];
  EXPECT_EQ(tileMap.name, "tiny_dungeon_map");
  EXPECT_EQ(tileMap.type, "TileMapNode");
  EXPECT_EQ(tileMap.tileMap.imagePath,
            "Resources/Kenney/TinyDungeon/Tilemap/tilemap_packed.png");
  EXPECT_EQ(tileMap.tileMap.firstGid, 1u);
  EXPECT_EQ(tileMap.tileMap.mapWidth, 32u);
  EXPECT_EQ(tileMap.tileMap.mapHeight, 20u);
  EXPECT_EQ(tileMap.tileMap.tileWidth, 16u);
  EXPECT_EQ(tileMap.tileMap.tileHeight, 16u);
  ASSERT_EQ(tileMap.tileMap.layers.size(), 3u);
  EXPECT_GE(tileMap.tileMap.layers[0].tiles.size(), 600u);
  EXPECT_TRUE(std::ranges::any_of(tileMap.tileMap.layers[0].tiles,
                                  [](const DL::TileMapTile &tile) {
                                    return tile.flipDiagonal;
                                  }));
}

TEST(SceneDescriptionTest, RejectsUnknownNodeTypes) {
  DL::SceneNodeDescription description;
  description.type = "MissingNode";

  EXPECT_THROW(DL::buildSceneNode(description, DL::SceneBuilderContext{}),
               std::runtime_error);
}

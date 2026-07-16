#include "debugui.h"
#include "app.h"
#include "cameranode.h"
#include "fogoverlaynode.h"
#include "light2dnode.h"
#include "lightnode.h"
#include "logger.h"
#include "meshnode.h"
#include "particlesystemnode.h"
#include "phongshapenode.h"
#include "planenode.h"
#include "scenenode.h"
#include "spriteanimationnode.h"
#include "spritebatchnode.h"
#include "spritenode.h"
#include "textnode.h"
#include "tilemapnode.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <utility>

namespace DL {

namespace {
bool frameStarted = false;
constexpr float kOverlayAlpha = 0.72f;
SceneNode *selectedNode = nullptr;
SceneNode *rotationEditorNode = nullptr;
glm::vec3 rotationEditorEulerDegrees(0.0f);

const char *levelLabel(LogLevel level) {
  switch (level) {
  case LogLevel::Trace:
    return "Trace";
  case LogLevel::Info:
    return "Info";
  case LogLevel::Warning:
    return "Warning";
  case LogLevel::Error:
    return "Error";
  }
  return "Info";
}

int levelIndex(LogLevel level) {
  switch (level) {
  case LogLevel::Trace:
    return 0;
  case LogLevel::Info:
    return 1;
  case LogLevel::Warning:
    return 2;
  case LogLevel::Error:
    return 3;
  }
  return 1;
}

LogLevel levelFromIndex(int index) {
  switch (index) {
  case 0:
    return LogLevel::Trace;
  case 1:
    return LogLevel::Info;
  case 2:
    return LogLevel::Warning;
  case 3:
    return LogLevel::Error;
  default:
    return LogLevel::Info;
  }
}

bool nodeExistsInSubtree(SceneNode *root, const SceneNode *candidate) {
  if (root == nullptr || candidate == nullptr) {
    return false;
  }
  if (root == candidate) {
    return true;
  }
  for (const auto &child : root->children) {
    if (nodeExistsInSubtree(child.get(), candidate)) {
      return true;
    }
  }
  return false;
}

const char *nodeLabel(SceneNode &node, std::size_t index) {
  if (!node.getDebugName().empty() && node.getDebugName() != "SceneNode") {
    return node.getDebugName().data();
  }
  if (!node.hasParent()) {
    return node.debugTypeName().data();
  }
  if (node.debugTypeName() != "SceneNode") {
    return node.debugTypeName().data();
  }
  thread_local std::string label;
  label = "Node ";
  label += std::to_string(index);
  return label.c_str();
}

void drawSceneNodeTree(SceneNode &node, std::size_t index = 0) {
  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
      ImGuiTreeNodeFlags_SpanAvailWidth;
  if (node.children.empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }
  if (selectedNode == &node) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  const bool open =
      ImGui::TreeNodeEx(static_cast<void *>(&node), flags, "%s", nodeLabel(node, index));
  if (ImGui::IsItemClicked()) {
    selectedNode = &node;
  }

  if (open) {
    for (std::size_t childIndex = 0; childIndex < node.children.size(); ++childIndex) {
      drawSceneNodeTree(*node.children[childIndex], childIndex);
    }
    ImGui::TreePop();
  }
}

const char *projectionLabel(CameraProjection projection) {
  switch (projection) {
  case CameraProjection::Perspective:
    return "Perspective";
  case CameraProjection::Orthographic:
    return "Orthographic";
  }
  return "Perspective";
}

const char *lightKindLabel(LightNode::Kind kind) {
  switch (kind) {
  case LightNode::Kind::Directional:
    return "Directional";
  }
  return "Directional";
}

const char *planeTypeLabel(PlaneNode::PlaneType type) {
  switch (type) {
  case PlaneNode::PlaneType::Simple:
    return "Simple";
  case PlaneNode::PlaneType::Spinner:
    return "Spinner";
  }
  return "Simple";
}

const char *shapeTypeLabel(ShapeType type) {
  switch (type) {
  case ShapeType::Cube:
    return "Cube";
  case ShapeType::Sphere:
    return "Sphere";
  case ShapeType::Cylinder:
    return "Cylinder";
  }
  return "Cube";
}

const char *textAlignmentLabel(TextAlignment alignment) {
  switch (alignment) {
  case TextAlignment::LEFT:
    return "Left";
  case TextAlignment::CENTER:
    return "Center";
  case TextAlignment::RIGHT:
    return "Right";
  }
  return "Center";
}

const char *textAnchorLabel(TextAnchor anchor) {
  switch (anchor) {
  case TextAnchor::TOP_LEFT:
    return "TopLeft";
  case TextAnchor::TOP_CENTER:
    return "TopCenter";
  case TextAnchor::TOP_RIGHT:
    return "TopRight";
  case TextAnchor::CENTER_LEFT:
    return "CenterLeft";
  case TextAnchor::CENTER:
    return "Center";
  case TextAnchor::CENTER_RIGHT:
    return "CenterRight";
  case TextAnchor::BOTTOM_LEFT:
    return "BottomLeft";
  case TextAnchor::BOTTOM_CENTER:
    return "BottomCenter";
  case TextAnchor::BOTTOM_RIGHT:
    return "BottomRight";
  }
  return "Center";
}

std::size_t aliveParticleCount(const ParticleSystemNode &node) {
  const auto &particles = node.getParticles();
  return static_cast<std::size_t>(
      std::count_if(particles.begin(), particles.end(),
                    [](const ParticleSystemNode::ParticleState &particle) {
                      return particle.alive;
                    }));
}

void drawCameraNodeInspector(CameraNode &node) {
  ImGui::TextUnformatted("CameraNode");
  bool active = node.active();
  if (ImGui::Checkbox("Active camera", &active)) {
    node.setActive(active);
  }

  int projectionIndex =
      node.projection() == CameraProjection::Orthographic ? 1 : 0;
  const char *projectionItems[] = {"Perspective", "Orthographic"};
  if (ImGui::Combo("Projection", &projectionIndex, projectionItems, 2)) {
    node.setProjection(projectionIndex == 1 ? CameraProjection::Orthographic
                                            : CameraProjection::Perspective);
  }

  float fov = node.fov();
  if (ImGui::DragFloat("FOV", &fov, 0.25f, 1.0f, 160.0f, "%.2f")) {
    node.setFov(fov);
  }
  float orthographicHeight = node.orthographicHeight();
  if (ImGui::DragFloat("Ortho height", &orthographicHeight, 0.05f, 0.01f,
                       1000.0f, "%.3f")) {
    node.setOrthographicHeight(orthographicHeight);
  }
  ImGui::Text("Current projection %s", projectionLabel(node.projection()));
}

void drawLightNodeInspector(LightNode &node) {
  ImGui::Text("LightNode %s", lightKindLabel(node.kind()));
  bool active = node.active();
  if (ImGui::Checkbox("Light active", &active)) {
    node.setActive(active);
  }
  glm::vec3 color = node.color();
  if (ImGui::ColorEdit3("Light color", glm::value_ptr(color))) {
    node.setColor(color);
  }
  float intensity = node.intensity();
  if (ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 20.0f, "%.3f")) {
    node.setIntensity(intensity);
  }
  float ambientStrength = node.ambientStrength();
  if (ImGui::DragFloat("Ambient", &ambientStrength, 0.01f, 0.0f, 2.0f,
                       "%.3f")) {
    node.setAmbientStrength(ambientStrength);
  }
  glm::vec3 direction = node.direction();
  if (ImGui::DragFloat3("Direction", glm::value_ptr(direction), 0.01f, -1.0f,
                        1.0f, "%.3f")) {
    node.setDirection(direction);
  }
}

void drawLight2DNodeInspector(Light2DNode &node) {
  ImGui::TextUnformatted("Light2DNode");
  Light2DNode::Config config = node.config();
  bool changed = ImGui::ColorEdit4("Color", glm::value_ptr(config.color));
  changed = ImGui::DragFloat("Radius", &config.radius, 0.01f, 0.0f, 100.0f,
                             "%.3f") ||
            changed;
  changed = ImGui::DragFloat("Intensity", &config.intensity, 0.01f, 0.0f,
                             10.0f, "%.3f") ||
            changed;
  changed = ImGui::DragFloat("Softness", &config.softness, 0.01f, 0.0f, 1.0f,
                             "%.3f") ||
            changed;
  changed = ImGui::DragFloat("Flicker amount", &config.flickerAmount, 0.01f,
                             0.0f, 1.0f, "%.3f") ||
            changed;
  changed = ImGui::DragFloat("Flicker speed", &config.flickerSpeed, 0.05f,
                             0.0f, 60.0f, "%.2f") ||
            changed;
  if (changed) {
    node.setConfig(config);
  }
  ImGui::Text("Current intensity %.3f", node.currentIntensity());
}

void drawMeshNodeInspector(MeshNode &node) {
  ImGui::TextUnformatted("MeshNode");
  ImGui::TextWrapped("Mesh %.*s", static_cast<int>(node.assetPath().size()),
                     node.assetPath().data());
  bool debugNormals = node.debugNormals();
  if (ImGui::Checkbox("Debug normals", &debugNormals)) {
    node.setDebugNormals(debugNormals);
  }
  const auto &settings = node.visualizerSettings();
  ImGui::Text("Ambient %.3f", settings.ambientStrength);
  ImGui::Text("Specular %.3f", settings.specularStrength);
  ImGui::Text("Shininess %.3f", settings.shininess);

  if (node.hasAnimations()) {
    ImGui::Separator();
    ImGui::TextUnformatted("Animation");
    bool playing = node.isAnimationPlaying();
    if (ImGui::Checkbox("Playing", &playing)) {
      node.setAnimationPlaying(playing);
    }
    bool looping = node.isAnimationLooping();
    if (ImGui::Checkbox("Looping", &looping)) {
      node.setAnimationLooping(looping);
    }
    float playbackSpeed = node.animationPlaybackSpeed();
    if (ImGui::DragFloat("Playback speed", &playbackSpeed, 0.01f, 0.0f, 4.0f,
                         "%.3f")) {
      node.setAnimationPlaybackSpeed(playbackSpeed);
    }
    const std::size_t clipIndex = node.animationClipIndex();
    ImGui::Text("Clip %zu / %zu", clipIndex, node.animationClipCount());
    ImGui::Text("Clip name %.*s",
                static_cast<int>(node.animationClipName(clipIndex).size()),
                node.animationClipName(clipIndex).data());
    ImGui::Text("Blend weight %.3f", node.animationBlendWeight());
  }
}

void drawSpriteNodeInspector(SpriteNode &node) {
  ImGui::TextUnformatted("SpriteNode");
  ImGui::TextWrapped("Image %.*s", static_cast<int>(node.imagePath().size()),
                     node.imagePath().data());
  bool billboard = node.billboardEnabled();
  if (ImGui::Checkbox("Billboard", &billboard)) {
    node.setBillboardEnabled(billboard);
  }
  glm::vec4 sourceRect = node.atlasSourceRectPixels();
  if (ImGui::DragFloat4("Source rect", glm::value_ptr(sourceRect), 1.0f,
                        -1.0f, 8192.0f, "%.1f")) {
    node.setAtlasSourceRectPixels(sourceRect);
  }
  glm::bvec3 flip = node.atlasFlip();
  bool flipX = flip.x;
  bool flipY = flip.y;
  bool flipDiagonal = flip.z;
  bool changed = ImGui::Checkbox("Flip X", &flipX);
  changed = ImGui::Checkbox("Flip Y", &flipY) || changed;
  changed = ImGui::Checkbox("Flip diagonal", &flipDiagonal) || changed;
  if (changed) {
    node.setAtlasFlip(flipX, flipY, flipDiagonal);
  }
}

void drawSpriteAnimationNodeInspector(SpriteAnimationNode &node) {
  ImGui::TextUnformatted("SpriteAnimationNode");
  const SpriteAnimationConfig &animation = node.animation();
  bool playing = animation.playing;
  if (ImGui::Checkbox("Animation playing", &playing)) {
    node.setAnimationPlaying(playing);
  }
  bool looping = animation.looping;
  if (ImGui::Checkbox("Animation looping", &looping)) {
    node.setAnimationLooping(looping);
  }
  float fps = animation.fps;
  if (ImGui::DragFloat("FPS", &fps, 0.1f, 0.1f, 60.0f, "%.2f")) {
    node.setAnimationFps(fps);
  }
  ImGui::Text("Frame %zu / %zu", node.currentFrameIndex(),
              animation.frames.size());
}

void drawTextNodeInspector(TextNode &node) {
  ImGui::TextUnformatted("TextNode");
  ImGui::TextWrapped("Text %.*s", static_cast<int>(node.text().size()),
                     node.text().data());
  bool billboard = node.billboardEnabled();
  if (ImGui::Checkbox("Billboard", &billboard)) {
    node.setBillboardEnabled(billboard);
  }
  float fontHeight = node.fontPixelHeight();
  if (ImGui::DragFloat("Font px", &fontHeight, 1.0f, 1.0f, 256.0f, "%.1f")) {
    node.setFontPixelHeight(fontHeight);
  }
  glm::vec4 textColor = node.textColor();
  if (ImGui::ColorEdit4("Text color", glm::value_ptr(textColor))) {
    node.setTextColor(textColor);
  }
  glm::vec4 shadowColor = node.shadowColor();
  if (ImGui::ColorEdit4("Shadow color", glm::value_ptr(shadowColor))) {
    node.setShadowColor(shadowColor);
  }
  glm::vec2 shadowOffset = node.shadowOffset();
  if (ImGui::DragFloat2("Shadow offset", glm::value_ptr(shadowOffset), 0.1f,
                        -64.0f, 64.0f, "%.2f")) {
    node.setShadowOffset(shadowOffset);
  }
  ImGui::Text("Alignment %s", textAlignmentLabel(node.textAlignment()));
  ImGui::Text("Anchor %s", textAnchorLabel(node.textAnchor()));
}

void drawPlaneNodeInspector(PlaneNode &node) {
  ImGui::Text("PlaneNode %s", planeTypeLabel(node.planeType));
  int planeIndex = node.planeType == PlaneNode::PlaneType::Spinner ? 1 : 0;
  const char *planeItems[] = {"Simple", "Spinner"};
  if (ImGui::Combo("Plane type", &planeIndex, planeItems, 2)) {
    node.planeType = planeIndex == 1 ? PlaneNode::PlaneType::Spinner
                                     : PlaneNode::PlaneType::Simple;
  }
  glm::vec4 color = node.color;
  if (ImGui::ColorEdit4("Color", glm::value_ptr(color))) {
    node.color = color;
  }
}

void drawPhongShapeNodeInspector(PhongShapeNode &node) {
  ImGui::Text("PhongShapeNode %s", shapeTypeLabel(node.shapeType()));
  DL::PhongMaterial material = node.material();
  bool changed = ImGui::ColorEdit3("Diffuse", glm::value_ptr(material.diffuse));
  changed = ImGui::ColorEdit3("Ambient", glm::value_ptr(material.ambient)) ||
            changed;
  changed = ImGui::ColorEdit3("Specular", glm::value_ptr(material.specular)) ||
            changed;
  changed = ImGui::DragFloat("Shininess", &material.shininess, 0.25f, 1.0f,
                             256.0f, "%.2f") ||
            changed;
  if (changed) {
    node.setMaterial(material);
  }
}

void drawParticleSystemNodeInspector(ParticleSystemNode &node) {
  ImGui::TextUnformatted("ParticleSystemNode");
  bool billboard = node.isBillboardEnabled();
  if (ImGui::Checkbox("Billboard", &billboard)) {
    node.setBillboardEnabled(billboard);
  }
  const auto &config = node.getConfig();
  ImGui::Text("Alive particles %zu / %zu", aliveParticleCount(node),
              node.getParticles().size());
  ImGui::Text("Max particles %d", config.emission.maxParticles);
  ImGui::Text("Emission rate %.2f", config.emission.rate);
  ImGui::Text("Life %.2f - %.2f", config.life.min, config.life.max);
}

void drawTileMapNodeInspector(TileMapNode &node) {
  ImGui::TextUnformatted("TileMapNode");
  const DL::TileMapConfig &config = node.config();
  ImGui::TextWrapped("Image %s", config.imagePath.c_str());
  ImGui::Text("Map %u x %u", config.mapWidth, config.mapHeight);
  ImGui::Text("Tile %u x %u px", config.tileWidth, config.tileHeight);
  ImGui::Text("Columns %u", config.columns);
  ImGui::Text("Tile world size %.3f", config.tileWorldSize);
  ImGui::Text("Layers %zu", config.layers.size());
}

void drawSpriteBatchNodeInspector(SpriteBatchNode &node) {
  ImGui::TextUnformatted("SpriteBatchNode");
  const DL::SpriteBatchConfig &config = node.config();
  ImGui::TextWrapped("Image %s", config.imagePath.c_str());
  ImGui::Text("Sprites %zu", config.sprites.size());
}

void drawFogOverlayNodeInspector(FogOverlayNode &node) {
  ImGui::TextUnformatted("FogOverlayNode");
  FogOverlayNode::Config config = node.config();
  ImGui::TextWrapped("Image %s", config.imagePath.c_str());
  bool changed = ImGui::ColorEdit3("Color", glm::value_ptr(config.color));
  changed = ImGui::DragFloat("Alpha", &config.alpha, 0.01f, 0.0f, 1.0f,
                             "%.3f") ||
            changed;
  changed = ImGui::DragFloat("Softness", &config.softness, 0.01f, 0.0f, 1.0f,
                             "%.3f") ||
            changed;
  changed = ImGui::DragFloat2("Tiling", glm::value_ptr(config.tiling), 0.01f,
                              0.001f, 32.0f, "%.3f") ||
            changed;
  changed = ImGui::DragFloat2("Scroll speed",
                              glm::value_ptr(config.scrollSpeed), 0.001f,
                              -2.0f, 2.0f, "%.3f") ||
            changed;
  changed = ImGui::DragFloat("Second layer", &config.secondLayerStrength,
                             0.01f, 0.0f, 1.0f, "%.3f") ||
            changed;
  changed = ImGui::DragFloat2(
                "Second scroll", glm::value_ptr(config.secondLayerScrollSpeed),
                0.001f, -2.0f, 2.0f, "%.3f") ||
            changed;
  changed = ImGui::DragFloat("Pulse amount", &config.pulseAmount, 0.01f,
                             0.0f, 1.0f, "%.3f") ||
            changed;
  changed = ImGui::DragFloat("Pulse speed", &config.pulseSpeed, 0.01f, 0.0f,
                             10.0f, "%.3f") ||
            changed;
  if (changed) {
    node.setConfig(std::move(config));
  }
}

void drawTypedNodeInspector(SceneNode &node) {
  if (auto *cameraNode = dynamic_cast<CameraNode *>(&node)) {
    drawCameraNodeInspector(*cameraNode);
  } else if (auto *lightNode = dynamic_cast<LightNode *>(&node)) {
    drawLightNodeInspector(*lightNode);
  } else if (auto *light2DNode = dynamic_cast<Light2DNode *>(&node)) {
    drawLight2DNodeInspector(*light2DNode);
  } else if (auto *meshNode = dynamic_cast<MeshNode *>(&node)) {
    drawMeshNodeInspector(*meshNode);
  } else if (auto *spriteAnimationNode =
                 dynamic_cast<SpriteAnimationNode *>(&node)) {
    drawSpriteAnimationNodeInspector(*spriteAnimationNode);
    ImGui::Separator();
    ImGui::TextUnformatted("Sprite");
    drawSpriteNodeInspector(*spriteAnimationNode);
  } else if (auto *spriteNode = dynamic_cast<SpriteNode *>(&node)) {
    drawSpriteNodeInspector(*spriteNode);
  } else if (auto *spriteBatchNode = dynamic_cast<SpriteBatchNode *>(&node)) {
    drawSpriteBatchNodeInspector(*spriteBatchNode);
  } else if (auto *fogOverlayNode = dynamic_cast<FogOverlayNode *>(&node)) {
    drawFogOverlayNodeInspector(*fogOverlayNode);
  } else if (auto *textNode = dynamic_cast<TextNode *>(&node)) {
    drawTextNodeInspector(*textNode);
  } else if (auto *tileMapNode = dynamic_cast<TileMapNode *>(&node)) {
    drawTileMapNodeInspector(*tileMapNode);
  } else if (auto *planeNode = dynamic_cast<PlaneNode *>(&node)) {
    drawPlaneNodeInspector(*planeNode);
  } else if (auto *shapeNode = dynamic_cast<PhongShapeNode *>(&node)) {
    drawPhongShapeNodeInspector(*shapeNode);
  } else if (auto *particleNode = dynamic_cast<ParticleSystemNode *>(&node)) {
    drawParticleSystemNodeInspector(*particleNode);
  } else {
    ImGui::TextUnformatted("No typed properties.");
  }
}

void drawNodeInspector(SceneNode &node) {
  glm::vec3 localPosition = node.getLocalPosition();
  glm::vec3 localScale = node.getLocalScale();
  if (rotationEditorNode != &node) {
    rotationEditorNode = &node;
    rotationEditorEulerDegrees =
        glm::degrees(glm::eulerAngles(node.getLocalRotation()));
  }

  ImGui::Text("Label %s", nodeLabel(node, 0));
  ImGui::Text("Children %zu", node.children.size());
  ImGui::Text("Render components %zu", node.renderComponentCount());
  bool debugOverride = node.isDebugTransformOverrideEnabled();
  if (ImGui::Checkbox("Override animated transform", &debugOverride)) {
    node.setDebugTransformOverrideEnabled(debugOverride);
  }
  ImGui::Separator();

  if (ImGui::DragFloat3("Local position", glm::value_ptr(localPosition), 0.05f)) {
    if (node.isDebugTransformOverrideEnabled()) {
      node.setDebugLocalPosition(localPosition);
    } else {
      node.setLocalPosition(localPosition);
    }
  }
  if (ImGui::DragFloat3("Local rotation", glm::value_ptr(rotationEditorEulerDegrees),
                        0.5f)) {
    const glm::quat rotation = glm::quat(glm::radians(rotationEditorEulerDegrees));
    if (node.isDebugTransformOverrideEnabled()) {
      node.setDebugLocalRotation(rotation);
    } else {
      node.setLocalRotation(rotation);
    }
  }
  if (ImGui::DragFloat3("Local scale", glm::value_ptr(localScale), 0.05f, 0.001f, 100.0f)) {
    if (node.isDebugTransformOverrideEnabled()) {
      node.setDebugLocalScale(localScale);
    } else {
      node.setLocalScale(localScale);
    }
  }

  if (ImGui::Button("Reset transform")) {
    if (node.isDebugTransformOverrideEnabled()) {
      node.setDebugLocalPosition(glm::vec3(0.0f));
      node.setDebugLocalRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
      node.setDebugLocalScale(glm::vec3(1.0f));
    } else {
      node.setLocalPosition(glm::vec3(0.0f));
      node.setLocalRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
      node.setLocalScale(glm::vec3(1.0f));
    }
    rotationEditorEulerDegrees = glm::vec3(0.0f);
  }

  const glm::vec3 worldPosition = node.getWorldPosition();
  const glm::vec3 worldScale = node.getWorldScale();
  const glm::vec3 worldRotationEuler =
      glm::degrees(glm::eulerAngles(node.getWorldRotation()));

  ImGui::Separator();
  ImGui::Text("World position %.2f %.2f %.2f", worldPosition.x, worldPosition.y,
              worldPosition.z);
  ImGui::Text("World rotation %.1f %.1f %.1f", worldRotationEuler.x,
              worldRotationEuler.y, worldRotationEuler.z);
  ImGui::Text("World scale %.2f %.2f %.2f", worldScale.x, worldScale.y,
              worldScale.z);

  ImGui::Separator();
  ImGui::TextUnformatted("Node Properties");
  drawTypedNodeInspector(node);

  if (node.renderComponentCount() > 0) {
    ImGui::Separator();
    ImGui::TextUnformatted("Components");
    for (const auto &component : node.renderComponents()) {
      const std::string_view typeName = component->debugTypeName();
      ImGui::BulletText("%.*s", static_cast<int>(typeName.size()),
                        typeName.data());
    }
  }
}
}

void applyDebugUiStyle() {
#ifdef USE_IMGUI
  ImGui::StyleColorsLight();

  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 10.0f;
  style.ChildRounding = 8.0f;
  style.FrameRounding = 6.0f;
  style.PopupRounding = 8.0f;
  style.GrabRounding = 6.0f;
  style.ScrollbarRounding = 8.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;

  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_WindowBg] = ImVec4(0.97f, 0.98f, 0.99f, kOverlayAlpha);
  colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.18f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 0.99f, 0.90f);
  colors[ImGuiCol_Border] = ImVec4(0.68f, 0.74f, 0.80f, 0.55f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.89f, 0.92f, 0.96f, 0.76f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.88f, 0.95f, 0.88f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.92f, 0.95f, 0.98f, 0.80f);
  colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.68f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.80f, 0.88f, 0.96f, 0.78f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.74f, 0.84f, 0.95f, 0.86f);
  colors[ImGuiCol_Header] = ImVec4(0.70f, 0.82f, 0.94f, 0.42f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.55f, 0.75f, 0.95f, 0.66f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.71f, 0.94f, 0.82f);
  colors[ImGuiCol_Button] = ImVec4(0.52f, 0.74f, 0.96f, 0.54f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.67f, 0.94f, 0.78f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.34f, 0.61f, 0.90f, 0.88f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.86f, 0.90f, 0.95f, 0.30f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.56f, 0.67f, 0.78f, 0.55f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.46f, 0.60f, 0.76f, 0.72f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.54f, 0.72f, 0.86f);
  colors[ImGuiCol_Separator] = ImVec4(0.67f, 0.73f, 0.80f, 0.45f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.49f, 0.67f, 0.88f, 0.28f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.41f, 0.63f, 0.89f, 0.58f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.58f, 0.85f, 0.80f);
#endif
}

void beginDebugUiFrame() {
#ifdef USE_IMGUI
  if (frameStarted) {
    return;
  }
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  frameStarted = true;
#endif
}

void endDebugUiFrame() {
#ifdef USE_IMGUI
  frameStarted = false;
#endif
}

void drawFrameStatsOverlay(double frameTimeSeconds,
                           const RenderStats &renderStats) {
#ifdef USE_IMGUI
  beginDebugUiFrame();

  ImGui::SetNextWindowBgAlpha(kOverlayAlpha);
  ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
  ImGuiWindowFlags flags =
      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing;
  if (ImGui::Begin("Frame Stats", nullptr, flags)) {
    const float frameMs = static_cast<float>(frameTimeSeconds * 1000.0);
    const float fps =
        frameTimeSeconds > 0.0 ? static_cast<float>(1.0 / frameTimeSeconds)
                               : 0.0f;
    ImGui::Text("FPS %.1f", fps);
    ImGui::Text("Frame %.2f ms", frameMs);
    ImGui::Separator();
    ImGui::Text("Draw calls %u", renderStats.drawCalls);
    ImGui::Text("Triangles %u", renderStats.triangles);
    ImGui::Text("Pipeline switches %u", renderStats.pipelineSwitches);
    ImGui::Text("Texture binds %u", renderStats.textureBinds);
    ImGui::Text("Mesh binds %u", renderStats.meshBinds);
    ImGui::Text("Cached meshes %u", renderStats.meshCount);
    ImGui::Text("Cached textures %u", renderStats.textureCount);
    ImGui::Text("Cached shader programs %u", renderStats.pipelineCount);
  }
  ImGui::End();
#endif
}

void drawEngineDebugWindows(App &app, double frameTimeSeconds,
                            const RenderStats &renderStats) {
#ifdef USE_IMGUI
  beginDebugUiFrame();

  auto *rootNode = dynamic_cast<SceneNode *>(app.currentScene());
  if (selectedNode != nullptr &&
      (rootNode == nullptr || !nodeExistsInSubtree(rootNode, selectedNode))) {
    selectedNode = rootNode;
  } else if (selectedNode == nullptr) {
    selectedNode = rootNode;
  }

  ImGui::SetNextWindowSize(ImVec2(310.0f, 260.0f), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Engine")) {
    const float frameMs = static_cast<float>(frameTimeSeconds * 1000.0);
    const float fps =
        frameTimeSeconds > 0.0 ? static_cast<float>(1.0 / frameTimeSeconds)
                               : 0.0f;
    constexpr ImGuiTreeNodeFlags sectionFlags = ImGuiTreeNodeFlags_DefaultOpen;
    const glm::vec2 windowSize = app.windowSize();
    const glm::vec2 framebufferSize = app.framebufferSize();

    if (ImGui::CollapsingHeader("Timing", sectionFlags)) {
      bool paused = app.simulationPaused();
      if (ImGui::Checkbox("Pause fixed step", &paused)) {
        app.setSimulationPaused(paused);
      }
      ImGui::SameLine();
      if (ImGui::Button("Step")) {
        app.requestSimulationStep();
      }
      ImGui::Text("FPS %.1f", fps);
      ImGui::Text("Frame %.2f ms", frameMs);
      ImGui::Text("Fixed step %.2f ms", app.fixedTimeStep() * 1000.0f);
      ImGui::Text("Fixed updates last frame %d", app.lastFixedUpdateCount());
    }

    if (ImGui::CollapsingHeader("Window", sectionFlags)) {
      ImGui::Text("Window %.0f x %.0f", windowSize.x, windowSize.y);
      ImGui::Text("Framebuffer %.0f x %.0f", framebufferSize.x,
                  framebufferSize.y);
    }

    if (ImGui::CollapsingHeader("Rendering", sectionFlags)) {
      bool renderCullingEnabled = app.renderCullingEnabled();
      if (ImGui::Checkbox("Render culling", &renderCullingEnabled)) {
        app.setRenderCullingEnabled(renderCullingEnabled);
      }
      const RenderQueueStats queueStats = app.lastRenderQueueStats();
      ImGui::Text("Render items submitted %u", queueStats.submittedItems);
      ImGui::Text("Render items drawn %u", queueStats.drawnItems);
      ImGui::Text("Render items culled %u", queueStats.culledItems);
      ImGui::Text("Draw calls %u", renderStats.drawCalls);
      ImGui::Text("Triangles %u", renderStats.triangles);
      ImGui::Text("Pipeline switches %u", renderStats.pipelineSwitches);
      ImGui::Text("Texture binds %u", renderStats.textureBinds);
      ImGui::Text("Mesh binds %u", renderStats.meshBinds);
      ImGui::Text("Meshes %u", renderStats.meshCount);
      ImGui::Text("Textures %u", renderStats.textureCount);
      ImGui::Text("Pipelines %u", renderStats.pipelineCount);
    }

    if (ImGui::CollapsingHeader("Post Process", sectionFlags)) {
      bool postProcessEnabled = app.postProcessEnabled();
      if (ImGui::Checkbox("Enabled", &postProcessEnabled)) {
        app.setPostProcessEnabled(postProcessEnabled);
      }

      bool chromaticEnabled = app.chromaticEnabled();
      if (ImGui::Checkbox("Chromatic aberration", &chromaticEnabled)) {
        app.setChromaticEnabled(chromaticEnabled);
      }
      float chromaticStrength = app.chromaticStrength();
      if (ImGui::SliderFloat("Chromatic strength", &chromaticStrength, 0.0f,
                             0.08f, "%.3f")) {
        app.setChromaticStrength(chromaticStrength);
      }

      bool bloomColorGradeEnabled = app.bloomColorGradeEnabled();
      if (ImGui::Checkbox("Bloom color grade", &bloomColorGradeEnabled)) {
        app.setBloomColorGradeEnabled(bloomColorGradeEnabled);
      }
      float bloomIntensity = app.bloomIntensity();
      if (ImGui::SliderFloat("Bloom intensity", &bloomIntensity, 0.0f, 1.5f,
                             "%.3f")) {
        app.setBloomIntensity(bloomIntensity);
      }
      float bloomThreshold = app.bloomThreshold();
      if (ImGui::SliderFloat("Bloom threshold", &bloomThreshold, 0.0f, 1.2f,
                             "%.3f")) {
        app.setBloomThreshold(bloomThreshold);
      }
      float colorGradeSaturation = app.colorGradeSaturation();
      if (ImGui::SliderFloat("Color saturation", &colorGradeSaturation, 0.0f,
                             2.0f, "%.3f")) {
        app.setColorGradeSaturation(colorGradeSaturation);
      }
      float colorGradeContrast = app.colorGradeContrast();
      if (ImGui::SliderFloat("Color contrast", &colorGradeContrast, 0.5f,
                             2.0f, "%.3f")) {
        app.setColorGradeContrast(colorGradeContrast);
      }
      float colorGradeWarmth = app.colorGradeWarmth();
      if (ImGui::SliderFloat("Color warmth", &colorGradeWarmth, -1.0f, 1.0f,
                             "%.3f")) {
        app.setColorGradeWarmth(colorGradeWarmth);
      }

      bool crtEnabled = app.crtEnabled();
      if (ImGui::Checkbox("CRT scanlines", &crtEnabled)) {
        app.setCrtEnabled(crtEnabled);
      }
      float crtScanlineStrength = app.crtScanlineStrength();
      if (ImGui::SliderFloat("Scanline strength", &crtScanlineStrength, 0.0f,
                             0.6f, "%.3f")) {
        app.setCrtScanlineStrength(crtScanlineStrength);
      }
      float crtVignetteStrength = app.crtVignetteStrength();
      if (ImGui::SliderFloat("CRT vignette", &crtVignetteStrength, 0.0f, 0.6f,
                             "%.3f")) {
        app.setCrtVignetteStrength(crtVignetteStrength);
      }
      float crtCurvature = app.crtCurvature();
      if (ImGui::SliderFloat("CRT curvature", &crtCurvature, 0.0f, 1.2f,
                             "%.3f")) {
        app.setCrtCurvature(crtCurvature);
      }
      float crtWobble = app.crtWobble();
      if (ImGui::SliderFloat("CRT wobble", &crtWobble, 0.0f, 0.004f,
                             "%.4f")) {
        app.setCrtWobble(crtWobble);
      }
      float crtGrilleStrength = app.crtGrilleStrength();
      if (ImGui::SliderFloat("CRT grille", &crtGrilleStrength, 0.0f, 0.4f,
                             "%.3f")) {
        app.setCrtGrilleStrength(crtGrilleStrength);
      }
      float crtBrightness = app.crtBrightness();
      if (ImGui::SliderFloat("CRT brightness", &crtBrightness, 0.5f, 3.0f,
                             "%.2f")) {
        app.setCrtBrightness(crtBrightness);
      }
    }

    if (ImGui::CollapsingHeader("Audio", sectionFlags)) {
      ImGui::Text("Audio clips %zu", app.audioSystem().loadedClipCount());
      ImGui::Text("Audio sounds %zu", app.audioSystem().activeSoundCount());
      float masterVolume = app.audioSystem().masterVolume();
      if (ImGui::SliderFloat("Master volume", &masterVolume, 0.0f, 2.0f,
                             "%.2f")) {
        app.audioSystem().setMasterVolume(masterVolume);
      }
      const auto &spectrum = app.audioSystem().spectrumBands();
      ImGui::PlotHistogram("Spectrum", spectrum.data(),
                           static_cast<int>(spectrum.size()), 0, nullptr, 0.0f,
                           1.0f, ImVec2(0.0f, 72.0f));
      ImGui::TextUnformatted("Audio groups");
      for (int i = 0; i < static_cast<int>(DL::AudioGroup::Count); ++i) {
        const auto group = static_cast<DL::AudioGroup>(i);
        ImGui::PushID(i);
        bool muted = app.audioSystem().isGroupMuted(group);
        if (ImGui::Checkbox("Mute", &muted)) {
          app.audioSystem().setGroupMuted(group, muted);
        }
        ImGui::SameLine();
        ImGui::Text("%s (%zu)", DL::AudioSystem::groupLabel(group),
                    app.audioSystem().activeSoundCount(group));
        float groupVolume = app.audioSystem().groupVolume(group);
        if (ImGui::SliderFloat("Volume", &groupVolume, 0.0f, 2.0f, "%.2f")) {
          app.audioSystem().setGroupVolume(group, groupVolume);
        }
        ImGui::PopID();
      }
    }
  }
  ImGui::End();

  if (rootNode != nullptr) {
    ImGui::SetNextWindowSize(ImVec2(280.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Tree")) {
      drawSceneNodeTree(*rootNode);
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(360.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Inspector")) {
      if (selectedNode != nullptr) {
        drawNodeInspector(*selectedNode);
      } else {
        ImGui::TextUnformatted("No node selected.");
      }
    }
    ImGui::End();
  }
#endif
}

void drawLogWindow() {
#ifdef USE_IMGUI
  beginDebugUiFrame();

  static bool visible = true;
  static bool autoScroll = true;
  static char filterBuffer[128] = "";

  if (!visible) {
    return;
  }

  ImGui::SetNextWindowBgAlpha(kOverlayAlpha);
  ImGui::SetNextWindowSize(ImVec2(620.0f, 280.0f), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Logs", &visible)) {
    ImGui::End();
    return;
  }

  auto &logger = Logger::instance();
  bool enabled = logger.isEnabled();
  if (ImGui::Checkbox("Enabled", &enabled)) {
    logger.setEnabled(enabled);
  }
  ImGui::SameLine();
  bool consoleEcho = logger.isConsoleEchoEnabled();
  if (ImGui::Checkbox("Console echo", &consoleEcho)) {
    logger.setConsoleEchoEnabled(consoleEcho);
  }
  ImGui::SameLine();
  bool fileSink = logger.isFileSinkEnabled();
  if (ImGui::Checkbox("Persist to file", &fileSink)) {
    logger.setFileSinkEnabled(fileSink);
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    logger.clear();
  }
  ImGui::SameLine();
  ImGui::Checkbox("Auto-scroll", &autoScroll);

  int minLevel = levelIndex(logger.minimumLevel());
  const char *levelLabels[] = {"Trace", "Info", "Warning", "Error"};
  ImGui::PushItemWidth(140.0f);
  if (ImGui::Combo("Min level", &minLevel, levelLabels,
                   IM_ARRAYSIZE(levelLabels))) {
    logger.setMinimumLevel(levelFromIndex(minLevel));
  }
  ImGui::PopItemWidth();
  ImGui::Text("File %s", logger.logFilePath().c_str());
  ImGui::PushItemWidth(240.0f);
  ImGui::InputText("Filter", filterBuffer, IM_ARRAYSIZE(filterBuffer));
  ImGui::PopItemWidth();
  ImGui::Separator();

  const std::string_view filterText(filterBuffer);
  const auto entries = logger.snapshot();
  ImGui::BeginChild("LogEntries");
  for (const auto &entry : entries) {
    if (!filterText.empty() &&
        entry.formatted.find(filterText) == std::string::npos &&
        entry.subsystem.find(filterText) == std::string::npos &&
        entry.message.find(filterText) == std::string::npos &&
        entry.detail.find(filterText) == std::string::npos) {
      continue;
    }
    ImGui::TextUnformatted(entry.formatted.c_str());
  }
  if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f) {
    ImGui::SetScrollHereY(1.0f);
  }
  ImGui::EndChild();
  ImGui::End();
#endif
}

} // namespace DL

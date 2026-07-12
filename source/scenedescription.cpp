#include "scenedescription.h"

#include "cameranode.h"
#include "meshnode.h"
#include "particlesystemnode.h"
#include "phongshapenode.h"
#include "planenode.h"
#include "spritenode.h"
#include "textnode.h"
#include "tilemapnode.h"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace DL {
namespace {

struct JsonValue {
  using Object = std::unordered_map<std::string, JsonValue>;
  using Array = std::vector<JsonValue>;

  std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value;
};

class JsonParser {
public:
  JsonParser(std::string_view source, std::string_view sourceName)
      : source_(source), sourceName_(sourceName) {}

  JsonValue parse() {
    JsonValue value = parseValue();
    skipWhitespace();
    if (!atEnd()) {
      fail("unexpected trailing input");
    }
    return value;
  }

private:
  JsonValue parseValue() {
    skipWhitespace();
    if (atEnd()) {
      fail("expected value");
    }

    const char c = peek();
    if (c == '{') {
      return JsonValue{parseObject()};
    }
    if (c == '[') {
      return JsonValue{parseArray()};
    }
    if (c == '"') {
      return JsonValue{parseString()};
    }
    if (c == 't') {
      consumeLiteral("true");
      return JsonValue{true};
    }
    if (c == 'f') {
      consumeLiteral("false");
      return JsonValue{false};
    }
    if (c == 'n') {
      consumeLiteral("null");
      return JsonValue{nullptr};
    }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
      return JsonValue{parseNumber()};
    }
    fail("expected value");
  }

  JsonValue::Object parseObject() {
    expect('{');
    JsonValue::Object object;
    skipWhitespace();
    if (consumeIf('}')) {
      return object;
    }

    while (true) {
      skipWhitespace();
      if (peek() != '"') {
        fail("expected object key");
      }
      std::string key = parseString();
      skipWhitespace();
      expect(':');
      object.emplace(std::move(key), parseValue());
      skipWhitespace();
      if (consumeIf('}')) {
        return object;
      }
      expect(',');
    }
  }

  JsonValue::Array parseArray() {
    expect('[');
    JsonValue::Array array;
    skipWhitespace();
    if (consumeIf(']')) {
      return array;
    }

    while (true) {
      array.push_back(parseValue());
      skipWhitespace();
      if (consumeIf(']')) {
        return array;
      }
      expect(',');
    }
  }

  std::string parseString() {
    expect('"');
    std::string result;
    while (!atEnd()) {
      const char c = advance();
      if (c == '"') {
        return result;
      }
      if (c == '\\') {
        if (atEnd()) {
          fail("unterminated escape sequence");
        }
        const char escaped = advance();
        switch (escaped) {
        case '"':
        case '\\':
        case '/':
          result.push_back(escaped);
          break;
        case 'b':
          result.push_back('\b');
          break;
        case 'f':
          result.push_back('\f');
          break;
        case 'n':
          result.push_back('\n');
          break;
        case 'r':
          result.push_back('\r');
          break;
        case 't':
          result.push_back('\t');
          break;
        default:
          fail("unsupported escape sequence");
        }
      } else {
        result.push_back(c);
      }
    }
    fail("unterminated string");
  }

  double parseNumber() {
    const std::size_t start = position_;
    if (peek() == '-') {
      advance();
    }
    consumeDigits();
    if (!atEnd() && peek() == '.') {
      advance();
      consumeDigits();
    }
    if (!atEnd() && (peek() == 'e' || peek() == 'E')) {
      advance();
      if (!atEnd() && (peek() == '+' || peek() == '-')) {
        advance();
      }
      consumeDigits();
    }

    const std::string_view token = source_.substr(start, position_ - start);
    const std::string tokenString(token);
    char *end = nullptr;
    const double value = std::strtod(tokenString.c_str(), &end);
    if (end != tokenString.c_str() + tokenString.size()) {
      fail("invalid number");
    }
    return value;
  }

  void consumeDigits() {
    const std::size_t start = position_;
    while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
      advance();
    }
    if (position_ == start) {
      fail("expected digit");
    }
  }

  void consumeLiteral(std::string_view literal) {
    if (source_.substr(position_, literal.size()) != literal) {
      fail("expected literal");
    }
    position_ += literal.size();
  }

  void skipWhitespace() {
    while (!atEnd() && std::isspace(static_cast<unsigned char>(peek()))) {
      advance();
    }
  }

  bool consumeIf(char expected) {
    if (!atEnd() && peek() == expected) {
      advance();
      return true;
    }
    return false;
  }

  void expect(char expected) {
    skipWhitespace();
    if (atEnd() || peek() != expected) {
      std::string message = "expected '";
      message.push_back(expected);
      message.push_back('\'');
      fail(message);
    }
    advance();
  }

  [[nodiscard]] bool atEnd() const { return position_ >= source_.size(); }
  [[nodiscard]] char peek() const { return source_[position_]; }
  char advance() { return source_[position_++]; }

  [[noreturn]] void fail(std::string_view message) const {
    std::size_t line = 1;
    std::size_t column = 1;
    for (std::size_t i = 0; i < position_ && i < source_.size(); ++i) {
      if (source_[i] == '\n') {
        ++line;
        column = 1;
      } else {
        ++column;
      }
    }

    std::ostringstream out;
    out << sourceName_ << ':' << line << ':' << column << ": " << message;
    throw std::runtime_error(out.str());
  }

  std::string_view source_;
  std::string_view sourceName_;
  std::size_t position_ = 0;
};

const JsonValue::Object &asObject(const JsonValue &value,
                                  std::string_view context) {
  if (const auto *object = std::get_if<JsonValue::Object>(&value.value)) {
    return *object;
  }
  throw std::runtime_error(std::string(context) + " must be an object");
}

const JsonValue::Array &asArray(const JsonValue &value,
                                std::string_view context) {
  if (const auto *array = std::get_if<JsonValue::Array>(&value.value)) {
    return *array;
  }
  throw std::runtime_error(std::string(context) + " must be an array");
}

const JsonValue *find(const JsonValue::Object &object, std::string_view key) {
  const auto it = object.find(std::string(key));
  return it != object.end() ? &it->second : nullptr;
}

std::string stringOr(const JsonValue::Object &object, std::string_view key,
                     std::string fallback) {
  const JsonValue *value = find(object, key);
  if (value == nullptr) {
    return fallback;
  }
  if (const auto *text = std::get_if<std::string>(&value->value)) {
    return *text;
  }
  throw std::runtime_error(std::string(key) + " must be a string");
}

bool boolOr(const JsonValue::Object &object, std::string_view key,
            bool fallback) {
  const JsonValue *value = find(object, key);
  if (value == nullptr) {
    return fallback;
  }
  if (const auto *flag = std::get_if<bool>(&value->value)) {
    return *flag;
  }
  throw std::runtime_error(std::string(key) + " must be a bool");
}

float floatOr(const JsonValue::Object &object, std::string_view key,
              float fallback) {
  const JsonValue *value = find(object, key);
  if (value == nullptr) {
    return fallback;
  }
  if (const auto *number = std::get_if<double>(&value->value)) {
    return static_cast<float>(*number);
  }
  throw std::runtime_error(std::string(key) + " must be a number");
}

std::uint32_t uint32Or(const JsonValue::Object &object, std::string_view key,
                       std::uint32_t fallback) {
  const JsonValue *value = find(object, key);
  if (value == nullptr) {
    return fallback;
  }
  const auto *number = std::get_if<double>(&value->value);
  if (number == nullptr || *number < 0.0) {
    throw std::runtime_error(std::string(key) + " must be a non-negative number");
  }
  return static_cast<std::uint32_t>(*number);
}

glm::vec3 vec3Or(const JsonValue::Object &object, std::string_view key,
                 glm::vec3 fallback) {
  const JsonValue *value = find(object, key);
  if (value == nullptr) {
    return fallback;
  }

  const auto &array = asArray(*value, key);
  if (array.size() != 3) {
    throw std::runtime_error(std::string(key) + " must have 3 numbers");
  }

  glm::vec3 result{0.0f};
  for (std::size_t i = 0; i < 3; ++i) {
    const auto *number = std::get_if<double>(&array[i].value);
    if (number == nullptr) {
      throw std::runtime_error(std::string(key) + " must have 3 numbers");
    }
    result[static_cast<int>(i)] = static_cast<float>(*number);
  }
  return result;
}

glm::vec2 vec2Or(const JsonValue::Object &object, std::string_view key,
                 glm::vec2 fallback) {
  const JsonValue *value = find(object, key);
  if (value == nullptr) {
    return fallback;
  }

  const auto &array = asArray(*value, key);
  if (array.size() != 2) {
    throw std::runtime_error(std::string(key) + " must have 2 numbers");
  }

  glm::vec2 result{0.0f};
  for (std::size_t i = 0; i < 2; ++i) {
    const auto *number = std::get_if<double>(&array[i].value);
    if (number == nullptr) {
      throw std::runtime_error(std::string(key) + " must have 2 numbers");
    }
    result[static_cast<int>(i)] = static_cast<float>(*number);
  }
  return result;
}

glm::vec4 vec4Or(const JsonValue::Object &object, std::string_view key,
                 glm::vec4 fallback) {
  const JsonValue *value = find(object, key);
  if (value == nullptr) {
    return fallback;
  }

  const auto &array = asArray(*value, key);
  if (array.size() != 4) {
    throw std::runtime_error(std::string(key) + " must have 4 numbers");
  }

  glm::vec4 result{0.0f};
  for (std::size_t i = 0; i < 4; ++i) {
    const auto *number = std::get_if<double>(&array[i].value);
    if (number == nullptr) {
      throw std::runtime_error(std::string(key) + " must have 4 numbers");
    }
    result[static_cast<int>(i)] = static_cast<float>(*number);
  }
  return result;
}

SceneAnimationDescription parseAnimation(const JsonValue::Object &object) {
  SceneAnimationDescription description;
  description.clip = stringOr(object, "clip", description.clip);
  description.playing = boolOr(object, "playing", description.playing);
  description.looping = boolOr(object, "looping", description.looping);
  description.playbackSpeed =
      floatOr(object, "playbackSpeed", description.playbackSpeed);
  return description;
}

MeshVisualizerSettings parseVisualizerSettings(const JsonValue::Object &object,
                                                MeshVisualizerSettings settings) {
  settings.lightDirection =
      glm::normalize(vec3Or(object, "lightDirection", settings.lightDirection));
  settings.lightColor = vec3Or(object, "lightColor", settings.lightColor);
  settings.ambientStrength =
      floatOr(object, "ambientStrength", settings.ambientStrength);
  settings.specularStrength =
      floatOr(object, "specularStrength", settings.specularStrength);
  settings.shininess = floatOr(object, "shininess", settings.shininess);
  return settings;
}

PhongMaterial parseMaterial(const JsonValue::Object &object,
                            PhongMaterial material) {
  material.diffuse = vec3Or(object, "diffuse", material.diffuse);
  material.ambient = vec3Or(object, "ambient", material.ambient);
  material.specular = vec3Or(object, "specular", material.specular);
  material.shininess = floatOr(object, "shininess", material.shininess);
  return material;
}

TileMapConfig parseTileMap(const JsonValue::Object &object) {
  static constexpr std::uint32_t kFlippedHorizontallyFlag = 0x80000000u;
  static constexpr std::uint32_t kFlippedVerticallyFlag = 0x40000000u;
  static constexpr std::uint32_t kFlippedDiagonallyFlag = 0x20000000u;
  static constexpr std::uint32_t kTileFlagMask =
      kFlippedHorizontallyFlag | kFlippedVerticallyFlag |
      kFlippedDiagonallyFlag;

  TileMapConfig config;
  config.imagePath = stringOr(object, "image", config.imagePath);
  config.firstGid = uint32Or(object, "firstGid", config.firstGid);
  if (config.firstGid == 0u) {
    throw std::runtime_error("tilemap firstGid must be greater than zero");
  }
  config.mapWidth = uint32Or(object, "mapWidth", config.mapWidth);
  config.mapHeight = uint32Or(object, "mapHeight", config.mapHeight);
  config.tileWidth = uint32Or(object, "tileWidth", config.tileWidth);
  config.tileHeight = uint32Or(object, "tileHeight", config.tileHeight);
  config.columns = uint32Or(object, "columns", config.columns);
  config.tileWorldSize =
      floatOr(object, "tileWorldSize", config.tileWorldSize);

  const auto *layers = find(object, "layers");
  if (layers == nullptr) {
    return config;
  }

  for (const auto &layerValue : asArray(*layers, "layers")) {
    const auto &layerObject = asObject(layerValue, "tilemap layer");
    TileMapLayer layer;
    layer.name = stringOr(layerObject, "name", layer.name);
    layer.z = floatOr(layerObject, "z", layer.z);
    const auto *dataValue = find(layerObject, "data");
    if (dataValue == nullptr) {
      config.layers.push_back(std::move(layer));
      continue;
    }

    const auto &data = asArray(*dataValue, "tilemap layer data");
    for (std::size_t index = 0; index < data.size(); ++index) {
      const auto *number = std::get_if<double>(&data[index].value);
      if (number == nullptr || *number < 0.0) {
        throw std::runtime_error("tilemap layer data must contain non-negative numbers");
      }
      const auto rawGid = static_cast<std::uint32_t>(*number);
      if (rawGid == 0u) {
        continue;
      }
      const std::uint32_t gid = rawGid & ~kTileFlagMask;
      if (gid == 0u || config.mapWidth == 0u) {
        continue;
      }
      if (gid < config.firstGid) {
        throw std::runtime_error("tilemap gid is below firstGid");
      }
      layer.tiles.push_back(
          {.tileIndex = gid - config.firstGid,
           .x = static_cast<std::uint32_t>(index % config.mapWidth),
           .y = static_cast<std::uint32_t>(index / config.mapWidth),
           .flipX = (rawGid & kFlippedHorizontallyFlag) != 0u,
           .flipY = (rawGid & kFlippedVerticallyFlag) != 0u,
           .flipDiagonal = (rawGid & kFlippedDiagonallyFlag) != 0u});
    }
    config.layers.push_back(std::move(layer));
  }

  return config;
}

SceneNodeDescription parseNodeDescription(const JsonValue &value) {
  const auto &object = asObject(value, "node");

  SceneNodeDescription description;
  description.name = stringOr(object, "name", description.name);
  description.type = stringOr(object, "type", description.type);
  description.mesh = stringOr(object, "mesh", description.mesh);
  description.image = stringOr(object, "image", description.image);
  description.sourceRect =
      vec4Or(object, "sourceRect", description.sourceRect);
  description.text = stringOr(object, "text", description.text);
  description.textAlignment =
      stringOr(object, "alignment", description.textAlignment);
  description.textAnchor = stringOr(object, "anchor", description.textAnchor);
  description.fontSize = floatOr(object, "fontSize", description.fontSize);
  description.textColor = vec4Or(object, "textColor", description.textColor);
  description.shadowColor =
      vec4Or(object, "shadowColor", description.shadowColor);
  description.shadowOffset =
      vec2Or(object, "shadowOffset", description.shadowOffset);
  description.plane = stringOr(object, "plane", description.plane);
  description.shape = stringOr(object, "shape", description.shape);
  description.particle = stringOr(object, "particle", description.particle);
  description.billboard = boolOr(object, "billboard", description.billboard);
  description.flipX = boolOr(object, "flipX", description.flipX);
  description.flipY = boolOr(object, "flipY", description.flipY);
  description.flipDiagonal =
      boolOr(object, "flipDiagonal", description.flipDiagonal);
  description.active = boolOr(object, "active", description.active);
  description.fov = floatOr(object, "fov", description.fov);
  if (const auto *lookAt = find(object, "lookAt")) {
    const JsonValue::Object wrapper{{"lookAt", *lookAt}};
    description.lookAt = vec3Or(wrapper, "lookAt", glm::vec3(0.0f));
  }
  description.color = vec4Or(object, "color", description.color);

  if (const auto *transform = find(object, "transform")) {
    const auto &transformObject = asObject(*transform, "transform");
    description.position =
        vec3Or(transformObject, "position", description.position);
    description.rotationEulerDegrees =
        vec3Or(transformObject, "rotationEuler", description.rotationEulerDegrees);
    description.scale = vec3Or(transformObject, "scale", description.scale);
  }

  if (const auto *visualizer = find(object, "visualizer")) {
    description.visualizerSettings = parseVisualizerSettings(
        asObject(*visualizer, "visualizer"), description.visualizerSettings);
  }

  if (const auto *material = find(object, "material")) {
    description.material =
        parseMaterial(asObject(*material, "material"), description.material);
  }

  if (const auto *animation = find(object, "animation")) {
    description.animation = parseAnimation(asObject(*animation, "animation"));
  }

  if (const auto *tileMap = find(object, "tileMap")) {
    description.tileMap = parseTileMap(asObject(*tileMap, "tileMap"));
  }

  if (const auto *children = find(object, "children")) {
    for (const auto &child : asArray(*children, "children")) {
      description.children.push_back(parseNodeDescription(child));
    }
  }

  return description;
}

void applyTransform(SceneNode &node, const SceneNodeDescription &description) {
  node.setLocalPosition(description.position);
  const glm::vec3 radians = glm::radians(description.rotationEulerDegrees);
  node.setLocalRotation(glm::quat(radians));
  node.setLocalScale(description.scale);
}

void applyAnimation(MeshNode &node,
                    const SceneAnimationDescription &description) {
  node.setAnimationPlaying(description.playing);
  node.setAnimationLooping(description.looping);
  node.setAnimationPlaybackSpeed(description.playbackSpeed);
  if (!description.clip.empty()) {
    node.setAnimationClipIndex(node.findAnimationClipIndex(description.clip, 0u));
  }
}

PlaneNode::PlaneType parsePlaneType(std::string_view value) {
  if (value == "Simple") {
    return PlaneNode::PlaneType::Simple;
  }
  if (value == "Spinner") {
    return PlaneNode::PlaneType::Spinner;
  }
  throw std::runtime_error("unknown PlaneNode plane value: " +
                           std::string(value));
}

ShapeType parseShapeType(std::string_view value) {
  if (value == "Cube") {
    return ShapeType::Cube;
  }
  if (value == "Sphere") {
    return ShapeType::Sphere;
  }
  if (value == "Cylinder") {
    return ShapeType::Cylinder;
  }
  throw std::runtime_error("unknown PhongShapeNode shape value: " +
                           std::string(value));
}

ParticleSystemNode::Config parseParticleConfig(std::string_view value) {
  if (value == "SoftGlowBurst") {
    return ParticleSystemNode::Config::softGlowBurst();
  }
  if (value == "WaterFountain") {
    return ParticleSystemNode::Config::waterFountain();
  }
  if (value == "Default") {
    return ParticleSystemNode::Config{};
  }
  throw std::runtime_error("unknown ParticleSystemNode particle value: " +
                           std::string(value));
}

TextAlignment parseTextAlignment(std::string_view value) {
  if (value == "Left") {
    return TextAlignment::LEFT;
  }
  if (value == "Center") {
    return TextAlignment::CENTER;
  }
  if (value == "Right") {
    return TextAlignment::RIGHT;
  }
  throw std::runtime_error("unknown TextNode alignment value: " +
                           std::string(value));
}

TextAnchor parseTextAnchor(std::string_view value) {
  if (value == "TopLeft") {
    return TextAnchor::TOP_LEFT;
  }
  if (value == "TopCenter") {
    return TextAnchor::TOP_CENTER;
  }
  if (value == "TopRight") {
    return TextAnchor::TOP_RIGHT;
  }
  if (value == "CenterLeft") {
    return TextAnchor::CENTER_LEFT;
  }
  if (value == "Center") {
    return TextAnchor::CENTER;
  }
  if (value == "CenterRight") {
    return TextAnchor::CENTER_RIGHT;
  }
  if (value == "BottomLeft") {
    return TextAnchor::BOTTOM_LEFT;
  }
  if (value == "BottomCenter") {
    return TextAnchor::BOTTOM_CENTER;
  }
  if (value == "BottomRight") {
    return TextAnchor::BOTTOM_RIGHT;
  }
  throw std::runtime_error("unknown TextNode anchor value: " +
                           std::string(value));
}

std::unique_ptr<SceneNode> buildPlainNode(
    const SceneNodeDescription &, SceneBuilderContext, SceneNode *parent) {
  return std::make_unique<SceneNode>(parent);
}

std::unique_ptr<SceneNode> buildCameraNode(
    const SceneNodeDescription &description, SceneBuilderContext,
    SceneNode *parent) {
  auto node = std::make_unique<CameraNode>(parent);
  node->setActive(description.active);
  node->setFov(description.fov);
  if (description.lookAt.has_value()) {
    node->setLookAtTarget(*description.lookAt);
  }
  return node;
}

std::unique_ptr<SceneNode> buildMeshNode(const SceneNodeDescription &description,
                                         SceneBuilderContext context,
                                         SceneNode *parent) {
  auto node = std::make_unique<MeshNode>(
      description.mesh, context.codeBook, context.renderDevice,
      context.meshAssetCache, context.renderResourceCache, parent,
      context.camera);
  node->setVisualizerSettings(description.visualizerSettings);
  return node;
}

std::unique_ptr<SceneNode> buildSpriteNode(
    const SceneNodeDescription &description, SceneBuilderContext context,
    SceneNode *parent) {
  auto node = std::make_unique<SpriteNode>(
      description.image, context.codeBook, context.renderDevice,
      context.renderResourceCache, parent, context.camera);
  node->setBillboardEnabled(description.billboard);
  node->setAtlasSourceRectPixels(description.sourceRect);
  node->setAtlasFlip(description.flipX, description.flipY,
                     description.flipDiagonal);
  return node;
}

std::unique_ptr<SceneNode> buildTextNode(const SceneNodeDescription &description,
                                         SceneBuilderContext context,
                                         SceneNode *parent) {
  auto node = std::make_unique<TextNode>(parent, description.text,
                                         context.renderDevice,
                                         context.renderResourceCache,
                                         context.camera);
  node->setBillboardEnabled(description.billboard);
  node->setTextAlignment(parseTextAlignment(description.textAlignment));
  node->setTextAnchor(parseTextAnchor(description.textAnchor));
  node->setFontPixelHeight(description.fontSize);
  node->setTextColor(description.textColor);
  node->setShadowColor(description.shadowColor);
  node->setShadowOffset(description.shadowOffset);
  return node;
}

std::unique_ptr<SceneNode> buildTileMapNode(
    const SceneNodeDescription &description, SceneBuilderContext context,
    SceneNode *parent) {
  return std::make_unique<TileMapNode>(
      description.tileMap, context.renderDevice, context.renderResourceCache,
      parent, context.camera);
}

std::unique_ptr<SceneNode> buildPlaneNode(
    const SceneNodeDescription &description, SceneBuilderContext context,
    SceneNode *parent) {
  auto node = std::make_unique<PlaneNode>(
      parent, context.camera, context.renderDevice, context.renderResourceCache);
  node->planeType = parsePlaneType(description.plane);
  node->color = description.color;
  return node;
}

std::unique_ptr<SceneNode> buildPhongShapeNode(
    const SceneNodeDescription &description, SceneBuilderContext context,
    SceneNode *parent) {
  auto node = std::make_unique<PhongShapeNode>(
      parseShapeType(description.shape), context.renderDevice,
      context.renderResourceCache, parent, context.camera);
  node->setMaterial(description.material);
  return node;
}

std::unique_ptr<SceneNode> buildParticleSystemNode(
    const SceneNodeDescription &description, SceneBuilderContext context,
    SceneNode *parent) {
  auto node = std::make_unique<ParticleSystemNode>(
      context.renderDevice, context.camera, context.renderResourceCache,
      parseParticleConfig(description.particle), parent);
  node->setBillboardEnabled(description.billboard);
  return node;
}

} // namespace

void SceneNodeFactory::registerNodeType(std::string type, Builder builder) {
  builders_[std::move(type)] = std::move(builder);
}

std::unique_ptr<SceneNode>
SceneNodeFactory::build(const SceneNodeDescription &description,
                        SceneBuilderContext context, SceneNode *parent) const {
  const auto it = builders_.find(description.type);
  if (it == builders_.end()) {
    throw std::runtime_error("unknown scene node type: " + description.type);
  }
  return it->second(description, context, parent);
}

bool SceneNodeFactory::hasNodeType(std::string_view type) const {
  return builders_.contains(std::string(type));
}

SceneNodeFactory createDefaultSceneNodeFactory() {
  SceneNodeFactory factory;
  factory.registerNodeType("SceneNode", buildPlainNode);
  factory.registerNodeType("CameraNode", buildCameraNode);
  factory.registerNodeType("MeshNode", buildMeshNode);
  factory.registerNodeType("SpriteNode", buildSpriteNode);
  factory.registerNodeType("TextNode", buildTextNode);
  factory.registerNodeType("TileMapNode", buildTileMapNode);
  factory.registerNodeType("PlaneNode", buildPlaneNode);
  factory.registerNodeType("PhongShapeNode", buildPhongShapeNode);
  factory.registerNodeType("ParticleSystemNode", buildParticleSystemNode);
  return factory;
}

SceneDescription parseSceneDescription(std::string_view source,
                                        std::string_view sourceName) {
  const JsonValue root = JsonParser(source, sourceName).parse();
  const auto &object = asObject(root, "scene document");

  SceneDescription description;
  description.name = stringOr(object, "name", description.name);
  const auto *nodes = find(object, "nodes");
  if (nodes == nullptr) {
    throw std::runtime_error("scene document must contain nodes");
  }
  for (const auto &node : asArray(*nodes, "nodes")) {
    description.nodes.push_back(parseNodeDescription(node));
  }
  return description;
}

SceneDescription loadSceneDescription(const std::filesystem::path &path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open scene description: " +
                             path.string());
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return parseSceneDescription(buffer.str(), path.string());
}

std::unique_ptr<SceneNode> buildSceneNode(const SceneNodeDescription &description,
                                          SceneBuilderContext context,
                                          SceneNode *parent) {
  const SceneNodeFactory factory = createDefaultSceneNodeFactory();
  std::unique_ptr<SceneNode> node = factory.build(description, context, parent);

  node->setDebugName(description.name);
  applyTransform(*node, description);

  node->init();

  if (auto *meshNode = dynamic_cast<MeshNode *>(node.get());
      meshNode != nullptr && description.animation.has_value()) {
    applyAnimation(*meshNode, *description.animation);
  }

  for (const auto &childDescription : description.children) {
    node->addChild(buildSceneNode(childDescription, context, node.get()));
  }

  return node;
}

SceneNode *findSceneNodeByName(SceneNode &root, std::string_view name) {
  if (root.getDebugName() == name) {
    return &root;
  }
  for (auto &child : root.children) {
    if (auto *found = findSceneNodeByName(*child, name)) {
      return found;
    }
  }
  return nullptr;
}

} // namespace DL

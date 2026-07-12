#include "scenedescription.h"

#include "meshnode.h"
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

SceneNodeDescription parseNodeDescription(const JsonValue &value) {
  const auto &object = asObject(value, "node");

  SceneNodeDescription description;
  description.name = stringOr(object, "name", description.name);
  description.type = stringOr(object, "type", description.type);
  description.mesh = stringOr(object, "mesh", description.mesh);

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

  if (const auto *animation = find(object, "animation")) {
    description.animation = parseAnimation(asObject(*animation, "animation"));
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

} // namespace

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
  std::unique_ptr<SceneNode> node;
  if (description.type == "SceneNode") {
    node = std::make_unique<SceneNode>(parent);
  } else if (description.type == "MeshNode") {
    node = std::make_unique<MeshNode>(
        description.mesh, context.codeBook, context.renderDevice,
        context.meshAssetCache, context.renderResourceCache, parent,
        context.camera);
  } else {
    throw std::runtime_error("unknown scene node type: " + description.type);
  }

  node->setDebugName(description.name);
  applyTransform(*node, description);

  if (auto *meshNode = dynamic_cast<MeshNode *>(node.get())) {
    meshNode->setVisualizerSettings(description.visualizerSettings);
  }

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

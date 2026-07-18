#include "deflektorishlevel.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace Deflektorish {
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

    const std::string token(source_.substr(start, position_ - start));
    char *end = nullptr;
    const double value = std::strtod(token.c_str(), &end);
    if (end != token.c_str() + token.size()) {
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

int intOr(const JsonValue::Object &object, std::string_view key, int fallback) {
  const JsonValue *value = find(object, key);
  if (value == nullptr) {
    return fallback;
  }
  if (const auto *number = std::get_if<double>(&value->value)) {
    if (std::floor(*number) != *number) {
      throw std::runtime_error(std::string(key) + " must be an integer");
    }
    return static_cast<int>(*number);
  }
  throw std::runtime_error(std::string(key) + " must be a number");
}

glm::ivec2 cellFromArray(const JsonValue &value, std::string_view context) {
  const auto &array = asArray(value, context);
  if (array.size() != 2) {
    throw std::runtime_error(std::string(context) + " must have 2 integers");
  }
  glm::ivec2 result{0};
  for (std::size_t i = 0; i < 2; ++i) {
    const auto *number = std::get_if<double>(&array[i].value);
    if (number == nullptr || std::floor(*number) != *number) {
      throw std::runtime_error(std::string(context) + " must have 2 integers");
    }
    result[static_cast<int>(i)] = static_cast<int>(*number);
  }
  return result;
}

glm::ivec2 cellOr(const JsonValue::Object &object, std::string_view key,
                  glm::ivec2 fallback) {
  const JsonValue *value = find(object, key);
  return value != nullptr ? cellFromArray(*value, key) : fallback;
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

void requireNoUnknownFields(const JsonValue::Object &object,
                            std::initializer_list<std::string_view> known,
                            std::string_view context) {
  for (const auto &[key, value] : object) {
    const bool found = std::find(known.begin(), known.end(), key) != known.end();
    if (!found) {
      throw std::runtime_error(std::string(context) + " has unknown field '" +
                               key + "'");
    }
  }
}

SourceConfig parseSource(const JsonValue::Object &object) {
  requireNoUnknownFields(object, {"cell", "angleDegrees"}, "source");
  SourceConfig source;
  source.cell = cellOr(object, "cell", source.cell);
  source.angleDegrees = floatOr(object, "angleDegrees", source.angleDegrees);
  return source;
}

ReflektorConfig parseReflektor(const JsonValue::Object &object) {
  requireNoUnknownFields(object, {"cell", "angleDegrees", "automatic", "speed"},
                         "reflektor");
  ReflektorConfig reflektor;
  reflektor.cell = cellOr(object, "cell", reflektor.cell);
  reflektor.angleDegrees =
      floatOr(object, "angleDegrees", reflektor.angleDegrees);
  reflektor.automatic = boolOr(object, "automatic", reflektor.automatic);
  reflektor.speed = floatOr(object, "speed", reflektor.speed);
  return reflektor;
}

TargetConfig parseTarget(const JsonValue::Object &object) {
  requireNoUnknownFields(object, {"cell"}, "target");
  TargetConfig target;
  target.cell = cellOr(object, "cell", target.cell);
  return target;
}

BlockerConfig parseBlocker(const JsonValue::Object &object) {
  requireNoUnknownFields(object, {"cell", "reflective"}, "blocker");
  BlockerConfig blocker;
  blocker.cell = cellOr(object, "cell", blocker.cell);
  blocker.reflective = boolOr(object, "reflective", blocker.reflective);
  return blocker;
}

PortalConfig parsePortal(const JsonValue::Object &object) {
  requireNoUnknownFields(object, {"entryCell", "exitCell", "phase"}, "portal");
  PortalConfig portal;
  portal.entryCell = cellOr(object, "entryCell", portal.entryCell);
  portal.exitCell = cellOr(object, "exitCell", portal.exitCell);
  portal.phase = floatOr(object, "phase", portal.phase);
  return portal;
}

FilterConfig parseFilter(const JsonValue::Object &object) {
  requireNoUnknownFields(object, {"cell", "angleDegrees", "automatic", "speed"},
                         "filter");
  FilterConfig filter;
  filter.cell = cellOr(object, "cell", filter.cell);
  filter.angleDegrees = floatOr(object, "angleDegrees", filter.angleDegrees);
  filter.automatic = boolOr(object, "automatic", filter.automatic);
  filter.speed = floatOr(object, "speed", filter.speed);
  return filter;
}

SplitterConfig parseSplitter(const JsonValue::Object &object) {
  requireNoUnknownFields(object, {"cell", "angleDegrees"}, "splitter");
  SplitterConfig splitter;
  splitter.cell = cellOr(object, "cell", splitter.cell);
  splitter.angleDegrees =
      floatOr(object, "angleDegrees", splitter.angleDegrees);
  return splitter;
}

} // namespace

glm::vec2 cellToPosition(const GridConfig &grid, glm::ivec2 cell) {
  return {cell.x * grid.tileSize + grid.origin.x,
          cell.y * grid.tileSize + grid.origin.y};
}

LevelConfig parseLevel(std::string_view source, std::string_view sourceName) {
  const JsonValue rootValue = JsonParser(source, sourceName).parse();
  const auto &root = asObject(rootValue, "level");
  requireNoUnknownFields(root,
                         {"name", "grid", "source", "explosionPoolSize",
                          "reflektors", "targets", "blockers", "portals",
                          "filters", "splitters"},
                         "level");

  LevelConfig level;
  level.name = stringOr(root, "name", level.name);
  level.explosionPoolSize =
      intOr(root, "explosionPoolSize", level.explosionPoolSize);
  if (level.explosionPoolSize <= 0) {
    throw std::runtime_error("explosionPoolSize must be greater than zero");
  }

  if (const JsonValue *gridValue = find(root, "grid")) {
    const auto &grid = asObject(*gridValue, "grid");
    requireNoUnknownFields(grid, {"tileSize", "origin"}, "grid");
    level.grid.tileSize = floatOr(grid, "tileSize", level.grid.tileSize);
    level.grid.origin = vec2Or(grid, "origin", level.grid.origin);
  }

  if (const JsonValue *sourceValue = find(root, "source")) {
    level.source = parseSource(asObject(*sourceValue, "source"));
  }

  if (const JsonValue *values = find(root, "reflektors")) {
    for (const JsonValue &value : asArray(*values, "reflektors")) {
      level.reflektors.push_back(parseReflektor(asObject(value, "reflektor")));
    }
  }
  if (const JsonValue *values = find(root, "targets")) {
    for (const JsonValue &value : asArray(*values, "targets")) {
      level.targets.push_back(parseTarget(asObject(value, "target")));
    }
  }
  if (const JsonValue *values = find(root, "blockers")) {
    for (const JsonValue &value : asArray(*values, "blockers")) {
      level.blockers.push_back(parseBlocker(asObject(value, "blocker")));
    }
  }
  if (const JsonValue *values = find(root, "portals")) {
    for (const JsonValue &value : asArray(*values, "portals")) {
      level.portals.push_back(parsePortal(asObject(value, "portal")));
    }
  }
  if (const JsonValue *values = find(root, "filters")) {
    for (const JsonValue &value : asArray(*values, "filters")) {
      level.filters.push_back(parseFilter(asObject(value, "filter")));
    }
  }
  if (const JsonValue *values = find(root, "splitters")) {
    for (const JsonValue &value : asArray(*values, "splitters")) {
      level.splitters.push_back(parseSplitter(asObject(value, "splitter")));
    }
  }

  if (level.reflektors.empty()) {
    throw std::runtime_error("level must define at least one reflektor");
  }
  return level;
}

LevelConfig loadLevel(const std::filesystem::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to open Deflektorish level: " +
                             path.string());
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return parseLevel(buffer.str(), path.string());
}

} // namespace Deflektorish

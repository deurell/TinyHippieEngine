#include "deflektorishsoundmap.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

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

void requireNoUnknownFields(const JsonValue::Object &object,
                            std::initializer_list<std::string_view> known,
                            std::string_view context) {
  for (const auto &[key, value] : object) {
    (void)value;
    const bool found = std::find(known.begin(), known.end(), key) != known.end();
    if (!found) {
      throw std::runtime_error(std::string(context) + " has unknown field '" +
                               key + "'");
    }
  }
}

Sound soundFromEventKey(std::string_view key) {
  if (key == "target_first_hit") {
    return Sound::TargetFirstHit;
  }
  if (key == "target_destroyed") {
    return Sound::TargetDestroyed;
  }
  if (key == "score_tick") {
    return Sound::ScoreTick;
  }
  if (key == "initials_letter_change") {
    return Sound::InitialsLetterChange;
  }
  if (key == "game_over") {
    return Sound::GameOver;
  }
  throw std::runtime_error("unknown Deflektorish sound event '" +
                           std::string(key) + "'");
}

SoundEventConfig parseSoundEvent(const JsonValue::Object &object,
                                 std::string_view context) {
  requireNoUnknownFields(object,
                         {"clip", "volume", "volumePerEnergy", "maxVolume",
                          "pitchMin", "pitchMax", "poolSize",
                          "maxConcurrent"},
                         context);

  SoundEventConfig event;
  event.clip = stringOr(object, "clip", event.clip);
  if (event.clip.empty()) {
    throw std::runtime_error(std::string(context) + " clip must not be empty");
  }
  event.volume = floatOr(object, "volume", event.volume);
  event.volumePerEnergy =
      floatOr(object, "volumePerEnergy", event.volumePerEnergy);
  event.maxVolume = floatOr(object, "maxVolume", event.maxVolume);
  event.pitchMin = floatOr(object, "pitchMin", event.pitchMin);
  event.pitchMax = floatOr(object, "pitchMax", event.pitchMax);
  event.poolSize = intOr(object, "poolSize", event.poolSize);
  event.maxConcurrent = intOr(object, "maxConcurrent", event.maxConcurrent);
  if (event.maxVolume < 0.0f || event.volume < 0.0f ||
      event.volumePerEnergy < 0.0f) {
    throw std::runtime_error(std::string(context) +
                             " volume values must be non-negative");
  }
  if (event.pitchMin <= 0.0f || event.pitchMax <= 0.0f ||
      event.pitchMin > event.pitchMax) {
    throw std::runtime_error(std::string(context) +
                             " pitch range must be positive and ordered");
  }
  if (event.poolSize <= 0) {
    throw std::runtime_error(std::string(context) +
                             " poolSize must be greater than zero");
  }
  if (event.maxConcurrent <= 0) {
    throw std::runtime_error(std::string(context) +
                             " maxConcurrent must be greater than zero");
  }
  return event;
}

} // namespace

const SoundEventConfig *SoundMapConfig::find(Sound sound) const {
  const auto it = events.find(sound);
  return it != events.end() ? &it->second : nullptr;
}

const char *soundEventKey(Sound sound) {
  switch (sound) {
  case Sound::TargetFirstHit:
    return "target_first_hit";
  case Sound::TargetDestroyed:
    return "target_destroyed";
  case Sound::ScoreTick:
    return "score_tick";
  case Sound::InitialsLetterChange:
    return "initials_letter_change";
  case Sound::GameOver:
    return "game_over";
  }
  return "unknown";
}

SoundMapConfig parseSoundMap(std::string_view source,
                             std::string_view sourceName) {
  const JsonValue rootValue = JsonParser(source, sourceName).parse();
  const auto &root = asObject(rootValue, "sound map");
  requireNoUnknownFields(root, {"audioRoot", "events"}, "sound map");

  SoundMapConfig soundMap;
  soundMap.audioRoot = stringOr(root, "audioRoot", soundMap.audioRoot);

  const JsonValue *eventsValue = find(root, "events");
  if (eventsValue == nullptr) {
    throw std::runtime_error("sound map must define events");
  }
  const auto &events = asObject(*eventsValue, "events");
  for (const auto &[key, value] : events) {
    soundMap.events[soundFromEventKey(key)] =
        parseSoundEvent(asObject(value, key), key);
  }
  if (soundMap.find(Sound::TargetFirstHit) == nullptr ||
      soundMap.find(Sound::TargetDestroyed) == nullptr) {
    throw std::runtime_error(
        "sound map must define target_first_hit and target_destroyed");
  }
  return soundMap;
}

SoundMapConfig loadSoundMap(const std::filesystem::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to open Deflektorish sound map: " +
                             path.string());
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return parseSoundMap(buffer.str(), path.string());
}

} // namespace Deflektorish

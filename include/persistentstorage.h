#pragma once

#include <optional>
#include <string>

namespace DL {

class PersistentStorage {
public:
  static std::optional<std::string> readText(const std::string &key);
  static bool writeText(const std::string &key, const std::string &value);
};

} // namespace DL

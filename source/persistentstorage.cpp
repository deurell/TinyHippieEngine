#include "persistentstorage.h"

#ifdef __EMSCRIPTEN__
#include <cstdlib>
#include <emscripten.h>
#else
#include <filesystem>
#include <fstream>
#include <sstream>
#endif

namespace DL {
namespace {

#ifndef __EMSCRIPTEN__
std::filesystem::path storagePath(const std::string &key) {
  std::string filename;
  filename.reserve(key.size() + 4);
  for (char c : key) {
    const bool valid =
        (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_';
    filename.push_back(valid ? c : '_');
  }
  if (filename.empty()) {
    filename = "data";
  }
  filename += ".txt";
  return std::filesystem::current_path() / ".tiny_hippie_storage" / filename;
}
#endif

} // namespace

std::optional<std::string> PersistentStorage::readText(const std::string &key) {
#ifdef __EMSCRIPTEN__
  char *value = static_cast<char *>(EM_ASM_PTR(
      {
        const key = UTF8ToString($0);
        const value = localStorage.getItem(key);
        if (value === null) {
          return 0;
        }
        return stringToNewUTF8(value);
      },
      key.c_str()));
  if (value == nullptr) {
    return std::nullopt;
  }
  std::string result(value);
  free(value);
  return result;
#else
  const std::filesystem::path path = storagePath(key);
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file) {
    return std::nullopt;
  }
  std::ostringstream stream;
  stream << file.rdbuf();
  return stream.str();
#endif
}

bool PersistentStorage::writeText(const std::string &key,
                                  const std::string &value) {
#ifdef __EMSCRIPTEN__
  return EM_ASM_INT(
             {
               try {
                 localStorage.setItem(UTF8ToString($0), UTF8ToString($1));
                 return 1;
               } catch (error) {
                 return 0;
               }
             },
             key.c_str(), value.c_str()) != 0;
#else
  const std::filesystem::path path = storagePath(key);
  std::error_code error;
  std::filesystem::create_directories(path.parent_path(), error);
  if (error) {
    return false;
  }
  std::ofstream file(path, std::ios::out | std::ios::binary |
                               std::ios::trunc);
  if (!file) {
    return false;
  }
  file << value;
  return static_cast<bool>(file);
#endif
}

} // namespace DL

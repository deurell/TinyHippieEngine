#pragma once

#include <chrono>
#include <cstdio>
#include <mutex>
#include <string_view>

namespace DL {

enum class LogLevel { Trace, Info, Warning, Error };

class Logger {
public:
  static Logger &instance();

  void log(LogLevel level, std::string_view message);
  void log(LogLevel level, std::string_view message, std::string_view detail);

private:
  Logger() = default;
  const char *levelToString(LogLevel level) const;
  std::mutex mutex_;
};

inline void LogTrace(std::string_view msg) {
  Logger::instance().log(LogLevel::Trace, msg);
}

inline void LogTrace(std::string_view msg, std::string_view detail) {
  Logger::instance().log(LogLevel::Trace, msg, detail);
}

inline void LogInfo(std::string_view msg) {
  Logger::instance().log(LogLevel::Info, msg);
}

inline void LogInfo(std::string_view msg, std::string_view detail) {
  Logger::instance().log(LogLevel::Info, msg, detail);
}

inline void LogWarn(std::string_view msg) {
  Logger::instance().log(LogLevel::Warning, msg);
}

inline void LogWarn(std::string_view msg, std::string_view detail) {
  Logger::instance().log(LogLevel::Warning, msg, detail);
}

inline void LogError(std::string_view msg) {
  Logger::instance().log(LogLevel::Error, msg);
}

inline void LogError(std::string_view msg, std::string_view detail) {
  Logger::instance().log(LogLevel::Error, msg, detail);
}

} // namespace DL

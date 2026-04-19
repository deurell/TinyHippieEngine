#pragma once

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace DL {

enum class LogLevel { Trace, Info, Warning, Error };

struct LogEntry {
  std::uint64_t sequence = 0;
  std::chrono::system_clock::time_point timestamp;
  LogLevel level = LogLevel::Info;
  std::string subsystem = "general";
  std::string message;
  std::string detail;
  std::string formatted;
};

class Logger {
public:
  static Logger &instance();

  [[nodiscard]] bool isEnabled() const;
  void setEnabled(bool enabled);
  [[nodiscard]] bool isConsoleEchoEnabled() const;
  void setConsoleEchoEnabled(bool enabled);
  [[nodiscard]] bool isFileSinkEnabled() const;
  void setFileSinkEnabled(bool enabled);
  [[nodiscard]] std::string logFilePath() const;
  [[nodiscard]] LogLevel minimumLevel() const;
  void setMinimumLevel(LogLevel level);
  void clear();
  [[nodiscard]] std::vector<LogEntry> snapshot() const;

  void log(LogLevel level, std::string_view message);
  void log(LogLevel level, std::string_view message, std::string_view detail);
  void logEvent(LogLevel level, std::string_view subsystem,
                std::string_view message, std::string_view detail = {});
  [[nodiscard]] bool shouldLog(LogLevel level) const;

private:
  Logger() = default;
  const char *levelToString(LogLevel level) const;
  std::string formatEntry(const LogEntry &entry) const;
  mutable std::mutex mutex_;
  std::vector<LogEntry> entries_;
  std::uint64_t nextSequence_ = 1;
  std::size_t maxEntries_ = 1000;
  bool enabled_ = true;
  bool consoleEchoEnabled_ = true;
  bool fileSinkEnabled_ = false;
  std::string logFilePath_ = "logs/engine.log";
  LogLevel minimumLevel_ = LogLevel::Info;
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

#include "logger.h"
#include <filesystem>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace DL {

Logger &Logger::instance() {
  static Logger instance;
  return instance;
}

bool Logger::isEnabled() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return enabled_;
}

void Logger::setEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  enabled_ = enabled;
}

bool Logger::isConsoleEchoEnabled() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return consoleEchoEnabled_;
}

void Logger::setConsoleEchoEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  consoleEchoEnabled_ = enabled;
}

bool Logger::isFileSinkEnabled() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return fileSinkEnabled_;
}

void Logger::setFileSinkEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  fileSinkEnabled_ = enabled;
  if (!fileSinkEnabled_) {
    return;
  }

  const std::filesystem::path filePath(logFilePath_);
  const auto parentPath = filePath.parent_path();
  if (!parentPath.empty()) {
    std::error_code error;
    std::filesystem::create_directories(parentPath, error);
  }
}

std::string Logger::logFilePath() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return logFilePath_;
}

LogLevel Logger::minimumLevel() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return minimumLevel_;
}

void Logger::setMinimumLevel(LogLevel level) {
  std::lock_guard<std::mutex> lock(mutex_);
  minimumLevel_ = level;
}

void Logger::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  entries_.clear();
}

std::vector<LogEntry> Logger::snapshot() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return entries_;
}

bool Logger::shouldLog(LogLevel level) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return enabled_ && static_cast<int>(level) >= static_cast<int>(minimumLevel_);
}

void Logger::log(LogLevel level, std::string_view message) {
  log(level, message, "");
}

void Logger::log(LogLevel level, std::string_view message,
                 std::string_view detail) {
  logEvent(level, "general", message, detail);
}

void Logger::logEvent(LogLevel level, std::string_view subsystem,
                      std::string_view message, std::string_view detail) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!enabled_ || static_cast<int>(level) < static_cast<int>(minimumLevel_)) {
    return;
  }

  LogEntry entry;
  entry.sequence = nextSequence_++;
  entry.timestamp = std::chrono::system_clock::now();
  entry.level = level;
  entry.subsystem = std::string(subsystem.empty() ? "general" : subsystem);
  entry.message = std::string(message);
  entry.detail = std::string(detail);
  entry.formatted = formatEntry(entry);

  if (entries_.size() >= maxEntries_) {
    entries_.erase(entries_.begin());
  }
  entries_.push_back(entry);

  if (consoleEchoEnabled_) {
    std::clog << entry.formatted << std::endl;
  }
  if (fileSinkEnabled_) {
    const std::filesystem::path filePath(logFilePath_);
    const auto parentPath = filePath.parent_path();
    if (!parentPath.empty()) {
      std::error_code error;
      std::filesystem::create_directories(parentPath, error);
    }

    std::ofstream output(logFilePath_, std::ios::app);
    if (output) {
      output << entry.formatted << '\n';
    }
  }
}

const char *Logger::levelToString(LogLevel level) const {
  switch (level) {
  case LogLevel::Trace:
    return "trace";
  case LogLevel::Info:
    return "info";
  case LogLevel::Warning:
    return "warn";
  case LogLevel::Error:
    return "error";
  default:
    return "log";
  }
}

std::string Logger::formatEntry(const LogEntry &entry) const {
  std::time_t nowTime = std::chrono::system_clock::to_time_t(entry.timestamp);
  std::tm tmStruct{};
#if defined(_WIN32)
  localtime_s(&tmStruct, &nowTime);
#else
  localtime_r(&nowTime, &tmStruct);
#endif

  std::ostringstream oss;
  oss << '[' << std::put_time(&tmStruct, "%H:%M:%S") << "] "
      << '[' << levelToString(entry.level) << "] "
      << '[' << entry.subsystem << "] " << entry.message;
  if (!entry.detail.empty()) {
    oss << " | " << entry.detail;
  }
  return oss.str();
}

} // namespace DL

#include "logger.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace DL {

Logger &Logger::instance() {
  static Logger instance;
  return instance;
}

void Logger::log(LogLevel level, std::string_view message) {
  log(level, message, "");
}

void Logger::log(LogLevel level, std::string_view message,
                 std::string_view detail) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto now = std::chrono::system_clock::now();
  std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
  std::tm tmStruct{};
#if defined(_WIN32)
  localtime_s(&tmStruct, &nowTime);
#else
  localtime_r(&nowTime, &tmStruct);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tmStruct, "%H:%M:%S");
  std::clog << '[' << oss.str() << "] [" << levelToString(level) << "] "
            << message;
  if (!detail.empty()) {
    std::clog << ": " << detail;
  }
  std::clog << std::endl;
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

} // namespace DL

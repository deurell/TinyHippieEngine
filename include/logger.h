#pragma once

#include <iostream>
namespace DL {
class Logger {
public:
  Logger() : m_id(0) {}

  void log(std::string message) { std::cout << message; }

private:
  unsigned int m_id;
};
} // namespace DL

#ifdef _WIN32
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "app.h"

int main() {
  DL::App app;
  return app.run();
}
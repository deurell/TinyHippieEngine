#include "app.h"
#include "game/starterbootstrap.h"

int main() {
  DL::App app(createStarterBootstrap());
  return app.run();
}

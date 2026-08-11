#include "app.h"
#include "game/deflektorishbootstrap.h"

int main() {
  DL::App app(createDeflektorishBootstrap());
  return app.run();
}

#pragma once

namespace DL {

class App;

class AppBootstrap {
public:
  virtual ~AppBootstrap() = default;
  virtual void configure(App &app) = 0;
  virtual void resourcesReady(App &app) {}
  virtual void update(App &app, float deltaTime) {}
  virtual void beforeRender(App &app) {}
};

} // namespace DL

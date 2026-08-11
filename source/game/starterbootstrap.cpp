#include "game/starterbootstrap.h"

#include "app.h"
#include "game/scenes/inputdebugscene.h"
#include "game/scenes/skeletalanimationblendscene.h"
#include "game/scenes/textstarterscene.h"
#ifdef TINY_ENGINE_ENABLE_PHYSICS
#include "game/scenes/physicstestscene.h"
#endif

namespace {

class StarterBootstrap final : public DL::AppBootstrap {
public:
  void configure(DL::App &app) override {
    registerTextScene(app, "Resources/Scenes/simple_starter.scene.json");
    app.registerScene([&app] {
      return std::make_unique<InputDebugScene>(app.renderDevice(),
                                               app.renderResourceCache());
    });
    registerTextScene(app,
                      "Resources/Scenes/tiny_dungeon_atlas.scene.json");
    registerTextScene(app,
                      "Resources/Scenes/kenney_platformer.scene.json");
    app.registerScene([&app] {
      return std::make_unique<SkeletalAnimationBlendScene>(
          app.renderDevice(), app.basisCodebook(), app.meshAssetCache(),
          app.renderResourceCache());
    });
#ifdef TINY_ENGINE_ENABLE_PHYSICS
    app.registerScene([&app] {
      return std::make_unique<PhysicsTestScene>(app.renderDevice());
    });
#endif
  }

private:
  static void registerTextScene(DL::App &app, const char *path) {
    app.registerScene([&app, path] {
      return std::make_unique<TextStarterScene>(
          app.renderDevice(), app.basisCodebook(), app.meshAssetCache(),
          app.renderResourceCache(), path);
    });
  }
};

} // namespace

std::unique_ptr<DL::AppBootstrap> createStarterBootstrap() {
  return std::make_unique<StarterBootstrap>();
}

#include "appbootstrap.h"
#include "audiosystem.h"
#include "camera.h"
#include "iscene.h"
#include "lighting.h"
#include "rendercomponent.h"
#include "renderdevice.h"
#include "renderpass.h"
#include "scenemanager.h"
#include "scenenode.h"

namespace {

class PublicScene final : public DL::IScene {
public:
  void init() override {}
  void update(const DL::FrameContext &) override {}
  void render(const DL::FrameContext &) override {}
  void onClick(double, double) override {}
  void onKey(int) override {}
  void onScreenSizeChanged(glm::vec2) override {}
};

class PublicBootstrap final : public DL::AppBootstrap {
public:
  void configure(DL::App &) override {}
};

} // namespace

int main() {
  DL::SceneManager scenes;
  scenes.registerScene([] { return std::make_unique<PublicScene>(); });
  DL::InputState input;
  DL::SceneNode root;
  PublicBootstrap bootstrap;
  return !scenes.hasScenes() || input.moveAxis.x != 0.0f || root.hasParent() ||
         sizeof(bootstrap) == 0;
}

#include "appbootstrap.h"
#include "audiosystem.h"
#include "iscene.h"
#include "rendercomponent.h"
#include "renderdevice.h"
#include "scenemanager.h"
#include "scenenode.h"

int main() {
  DL::SceneManager scenes;
  DL::InputState input;
  DL::SceneNode root;
  return scenes.hasScenes() || input.moveAxis.x != 0.0f || root.hasParent();
}

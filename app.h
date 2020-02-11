#include "camera.h"
#include "shader.h"
#include "texture.h"

#include <GLFW/glfw3.h>

#ifdef Emscripten
#include <emscripten.h>
#endif

#include <memory>

class App {
public:
  App() = default;
  ~App() = default;

  int run();

private:
  void processInput(GLFWwindow *window);
  void renderLoop();
  void basisInit();

  GLFWwindow *m_window;
  std::unique_ptr<Texture> mTexture;
  std::unique_ptr<Shader> mShader;
  std::unique_ptr<Shader> mLightingShader;
  std::unique_ptr<Shader> mLampShader;
  std::unique_ptr<Camera> mCamera;
  std::unique_ptr<basist::etc1_global_selector_codebook> m_codebook;

  unsigned int mLightVAO;
  unsigned int mCubeVAO;

  static constexpr float screenWidth = 1024;
  static constexpr float screenHeight = 768;

  glm::vec3 mLightPos = {1.2f, 1.0f, 2.0f};

  float deltaTime = 0.0f;
  float lastFrame = 0.0f;

  glm::vec3 model_translate;
  glm::vec3 obj_color = {0.3, 0.8, 0.3};
};
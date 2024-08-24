#include "imagevisualizer.h"

DL::ImageVisualizer::ImageVisualizer(
    std::string name, DL::Camera &camera, std::string_view glslVersionString,
    SceneNode &node, std::unique_ptr<DL::Texture> texture, basist::etc1_global_selector_codebook *codeBook,
    const std::function<void(DL::Shader &)> &shaderModifier,
    std::string vertexShaderPath, std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(name), std::string(glslVersionString),
                     vertexShaderPath, fragmentShaderPath, node),
      texture_(std::move(texture)), codeBook_(codeBook), shaderModifier_(shaderModifier) {

  camera.lookAt({0, 0, 0});

  const float vertices[] = {
      // positions        // colors         // texture coords
      1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      1.0f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
      -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // top left
  };

  const unsigned int indices[] = {0, 1, 3, 1, 2, 3};

  glGenVertexArrays(1, &VAO_);
  glGenBuffers(1, &VBO_);
  glGenBuffers(1, &EBO_);

  glBindVertexArray(VAO_);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)nullptr);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));

  glEnableVertexAttribArray(2);

  glBindVertexArray(0);
}

void DL::ImageVisualizer::render(const glm::mat4 &worldTransform, float delta) {
  shader_->use();
  glBindTexture(GL_TEXTURE_2D, texture_->mId);

  shader_->setFloat("iTime", (float)glfwGetTime());
  shader_->setInt("texture0", 0);
  glm::mat4 transform = glm::mat4(1.0f);
  glm::mat4 model = glm::mat4(1.0f);

  auto position = extractPosition(worldTransform);
  auto rotation = extractRotation(worldTransform);
  auto scale = extractScale(worldTransform);

  model = glm::translate(model, position);
  model = model * glm::mat4_cast(rotation);
  model = glm::scale(model, scale);

  shader_->setMat4f("model", model);
  glm::mat4 view = camera_.getViewMatrix();
  shader_->setMat4f("view", view);
  glm::mat4 projectionMatrix = camera_.getPerspectiveTransform();
  shader_->setMat4f("projection", projectionMatrix);

  if (shaderModifier_) {
    shaderModifier_(*shader_);
  }

  glBindVertexArray(VAO_);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

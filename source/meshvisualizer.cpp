#include "meshvisualizer.h"

#include <cstdint>

namespace DL {

MeshVisualizer::MeshVisualizer(
    std::string name, DL::Camera &camera, SceneNode &node, MeshAsset asset,
    basist::etc1_global_selector_codebook *codeBook,
    DL::IRenderDevice *renderDevice, std::string vertexShaderPath,
    std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(name), std::move(vertexShaderPath),
                     std::move(fragmentShaderPath), node),
      renderDevice_(renderDevice), codeBook_(codeBook), pipeline_(), asset_(std::move(asset)) {
  if (renderDevice_ == nullptr) {
    return;
  }

  pipeline_ = renderDevice_->createPipeline(vertexShaderPath_, fragmentShaderPath_);
  if (!pipeline_.valid()) {
    return;
  }

  submeshes_.reserve(asset_.submeshes.size());
  for (const auto &submesh : asset_.submeshes) {
    if (submesh.indices.empty()) {
      continue;
    }

    GpuSubmesh gpuSubmesh;
    gpuSubmesh.mesh = renderDevice_->createMesh(
        submesh.positions, submesh.normals, submesh.uvs, submesh.indices,
        submesh.jointIndices, submesh.jointWeights);
    gpuSubmesh.texture = loadTexture(submesh);
    gpuSubmesh.diffuseColor = submesh.diffuseColor;
    gpuSubmesh.ambientColor = submesh.ambientColor;
    gpuSubmesh.specularColor = submesh.specularColor;
    gpuSubmesh.shininess = submesh.shininess;
    gpuSubmesh.hasTexture = !submesh.texturePath.empty();
    gpuSubmesh.skinIndex = submesh.skinIndex;
    if (gpuSubmesh.mesh.valid() && gpuSubmesh.texture.valid()) {
      submeshes_.push_back(gpuSubmesh);
    } else {
      if (gpuSubmesh.mesh.valid()) {
        renderDevice_->destroy(gpuSubmesh.mesh);
      }
      if (gpuSubmesh.texture.valid()) {
        renderDevice_->destroy(gpuSubmesh.texture);
      }
    }
  }
}

MeshVisualizer::~MeshVisualizer() {
  if (renderDevice_ == nullptr) {
    return;
  }
  for (const auto &submesh : submeshes_) {
    if (submesh.mesh.valid()) {
      renderDevice_->destroy(submesh.mesh);
    }
    if (submesh.texture.valid()) {
      renderDevice_->destroy(submesh.texture);
    }
  }
  if (pipeline_.valid()) {
    renderDevice_->destroy(pipeline_);
  }
}

void MeshVisualizer::render(const glm::mat4 &worldTransform,
                            const DL::FrameContext &ctx) {
  if (renderDevice_ == nullptr || !pipeline_.valid()) {
    return;
  }

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, extractPosition(worldTransform));
  model = model * glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform));

  AnimationPose pose;
  if (animationEnabled_ && animationClipIndex_ < asset_.animations.size()) {
    pose = makeAnimationPose(asset_.nodes.size());
    evaluateAnimationClip(asset_.animations[animationClipIndex_], animationTime_, pose);
  }

  for (const auto &submesh : submeshes_) {
    if (!submesh.mesh.valid() || !submesh.texture.valid()) {
      continue;
    }

    DL::DrawCommand command;
    command.mesh = submesh.mesh;
    command.pipeline = pipeline_;
    command.texture = submesh.texture;
    if (!pose.nodes.empty() && submesh.skinIndex >= 0) {
      command.skinMatrices =
          computeSkinMatrices(asset_, submesh.skinIndex, pose);
    }
    command.uniforms.push_back(
        DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
    command.uniforms.push_back(DL::UniformValue::makeMat4("model", model));
    command.uniforms.push_back(
        DL::UniformValue::makeMat4("view", camera_.getViewMatrix()));
    command.uniforms.push_back(
        DL::UniformValue::makeMat4("projection", camera_.getPerspectiveTransform()));
    command.uniforms.push_back(
        DL::UniformValue::makeVec3("viewPos", camera_.getPosition()));
    command.uniforms.push_back(DL::UniformValue::makeVec3(
        "lightDirection", glm::normalize(settings_.lightDirection)));
    command.uniforms.push_back(DL::UniformValue::makeVec3(
        "lightColor", settings_.lightColor));
    command.uniforms.push_back(
        DL::UniformValue::makeFloat("ambientStrength", settings_.ambientStrength));
    command.uniforms.push_back(
        DL::UniformValue::makeFloat(
            "specularStrength",
            settings_.specularStrength *
                std::max({submesh.specularColor.r, submesh.specularColor.g,
                          submesh.specularColor.b})));
    command.uniforms.push_back(
        DL::UniformValue::makeFloat(
            "shininess",
            submesh.shininess > 0.0f ? submesh.shininess : settings_.shininess));
    command.uniforms.push_back(
        DL::UniformValue::makeVec3(
            "baseTint", submesh.hasTexture ? glm::vec3(1.0f) : submesh.diffuseColor));
    command.uniforms.push_back(
        DL::UniformValue::makeVec3(
            "ambientTint", submesh.hasTexture ? glm::vec3(1.0f) : submesh.ambientColor));
    command.uniforms.push_back(
        DL::UniformValue::makeInt("debugNormals", debugNormals_ ? 1 : 0));
    renderDevice_->draw(command);
  }
}

TextureHandle MeshVisualizer::createFallbackTexture() {
  const std::uint8_t pixel[] = {255, 255, 255, 255};
  return renderDevice_->createTexture({.pixels = pixel,
                                       .width = 1,
                                       .height = 1,
                                       .format = TextureFormat::RGBA8,
                                       .generateMipmaps = false});
}

TextureHandle MeshVisualizer::loadTexture(const MeshAssetSubmesh &submesh) {
  if (!submesh.texturePath.empty() && codeBook_ != nullptr &&
      submesh.texturePath.ends_with(".basis")) {
    auto texture =
        renderDevice_->createBasisTexture(submesh.texturePath, *codeBook_);
    if (texture.valid()) {
      return texture;
    }
  }
  return createFallbackTexture();
}

} // namespace DL

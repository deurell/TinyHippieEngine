#include "meshvisualizer.h"

#include "stb_image.h"
#include <algorithm>
#include <cstdint>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <unordered_map>

namespace DL {

MeshVisualizer::MeshVisualizer(
    DL::Camera &camera, SceneNode &node,
    std::shared_ptr<const MeshAsset> asset,
    basist::etc1_global_selector_codebook *codeBook,
    DL::IRenderDevice *renderDevice, DL::RenderResourceCache *resourceCache,
    std::string vertexShaderPath,
    std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(vertexShaderPath),
                     std::move(fragmentShaderPath), node),
      renderDevice_(renderDevice), resourceCache_(resourceCache),
      codeBook_(codeBook), pipeline_(), asset_(std::move(asset)) {
  if (renderDevice_ == nullptr || asset_ == nullptr) {
    return;
  }

  pipeline_ = resourceCache_ != nullptr
                  ? resourceCache_->acquirePipeline(vertexShaderPath_,
                                                    fragmentShaderPath_)
                  : renderDevice_->createPipeline(vertexShaderPath_,
                                                  fragmentShaderPath_);
  if (!pipeline_.valid()) {
    return;
  }

  submeshes_.reserve(asset_->submeshes.size());
  for (const auto &submesh : asset_->submeshes) {
    if (submesh.indices.empty()) {
      continue;
    }

    GpuSubmesh gpuSubmesh;
    gpuSubmesh.mesh = renderDevice_->createMesh(
        submesh.positions, submesh.normals, submesh.uvs, submesh.indices,
        submesh.jointIndices, submesh.jointWeights);
    bool sharedTexture = false;
    gpuSubmesh.texture = loadTexture(submesh, sharedTexture);
    gpuSubmesh.diffuseColor = submesh.diffuseColor;
    gpuSubmesh.ambientColor = submesh.ambientColor;
    gpuSubmesh.specularColor = submesh.specularColor;
    gpuSubmesh.shininess = submesh.shininess;
    gpuSubmesh.hasTexture =
        !submesh.texturePath.empty() || !submesh.encodedTextureData.empty();
    gpuSubmesh.sharedTexture = sharedTexture;
    gpuSubmesh.sourceNodeIndex = submesh.sourceNodeIndex;
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
    if (submesh.texture.valid() && !submesh.sharedTexture) {
      renderDevice_->destroy(submesh.texture);
    }
  }
  if (pipeline_.valid() && resourceCache_ == nullptr) {
    renderDevice_->destroy(pipeline_);
  }
}

void MeshVisualizer::updateAnimation(float deltaTime) {
  if (asset_ != nullptr) {
    animationPlayer_.update(asset_->animations, deltaTime);
    blendAnimationPlayer_.update(asset_->animations, deltaTime);
  }
}

std::size_t MeshVisualizer::findAnimationClipIndex(std::string_view name,
                                                   std::size_t fallback) const {
  if (asset_ == nullptr || asset_->animations.empty()) {
    return 0u;
  }
  for (std::size_t index = 0; index < asset_->animations.size(); ++index) {
    if (asset_->animations[index].name == name) {
      return index;
    }
  }
  return fallback < asset_->animations.size() ? fallback : 0u;
}

std::string_view MeshVisualizer::animationClipName(std::size_t index) const {
  if (asset_ == nullptr || index >= asset_->animations.size()) {
    return {};
  }
  return asset_->animations[index].name;
}

void MeshVisualizer::applyAnimationBlend(const AnimationBlendState &state) {
  animationPlayer_.setPlaying(state.playing);
  animationPlayer_.setLooping(state.looping);
  animationPlayer_.setPlaybackSpeed(state.playbackSpeed);
  setAnimationBlend(state.baseClipIndex, state.blendClipIndex, state.weight);
}

void MeshVisualizer::setAnimationBlend(std::size_t baseClipIndex,
                                       std::size_t blendClipIndex,
                                       float weight) {
  animationBlendWeight_ = std::clamp(weight, 0.0f, 1.0f);
  if (animationPlayer_.clipIndex() != baseClipIndex) {
    animationPlayer_.setClipIndex(baseClipIndex, false);
  }
  if (blendAnimationPlayer_.clipIndex() != blendClipIndex) {
    blendAnimationPlayer_.setClipIndex(blendClipIndex, false);
  }
}

void MeshVisualizer::setAnimationBlendByName(std::string_view baseClipName,
                                             std::string_view blendClipName,
                                             float weight) {
  setAnimationBlend(findAnimationClipIndex(baseClipName),
                    findAnimationClipIndex(blendClipName), weight);
}

AnimationPose MeshVisualizer::currentAnimationPose() const {
  if (asset_ == nullptr || asset_->animations.empty() ||
      animationPlayer_.clipIndex() >= asset_->animations.size()) {
    return {};
  }

  AnimationPose basePose = makeAnimationPose(asset_->nodes.size());
  evaluateAnimationClip(asset_->animations[animationPlayer_.clipIndex()],
                        animationPlayer_.time(), basePose);

  if (animationBlendWeight_ <= 0.0f ||
      blendAnimationPlayer_.clipIndex() >= asset_->animations.size()) {
    return basePose;
  }

  AnimationPose blendPose = makeAnimationPose(asset_->nodes.size());
  evaluateAnimationClip(asset_->animations[blendAnimationPlayer_.clipIndex()],
                        blendAnimationPlayer_.time(), blendPose);

  AnimationPose mixedPose = makeAnimationPose(asset_->nodes.size());
  const float weight = std::clamp(animationBlendWeight_, 0.0f, 1.0f);
  for (std::size_t nodeIndex = 0; nodeIndex < mixedPose.nodes.size(); ++nodeIndex) {
    const auto &assetNode = asset_->nodes[nodeIndex];
    const auto &baseNode = basePose.nodes[nodeIndex];
    const auto &blendNode = blendPose.nodes[nodeIndex];
    auto &mixedNode = mixedPose.nodes[nodeIndex];

    mixedNode.hasTranslation =
        baseNode.hasTranslation || blendNode.hasTranslation;
    if (mixedNode.hasTranslation) {
      const glm::vec3 baseTranslation =
          baseNode.hasTranslation ? baseNode.translation
                                  : assetNode.baseTranslation;
      const glm::vec3 blendTranslation =
          blendNode.hasTranslation ? blendNode.translation
                                   : assetNode.baseTranslation;
      mixedNode.translation = glm::mix(baseTranslation, blendTranslation, weight);
    }

    mixedNode.hasRotation = baseNode.hasRotation || blendNode.hasRotation;
    if (mixedNode.hasRotation) {
      const glm::quat baseRotation =
          baseNode.hasRotation ? baseNode.rotation : assetNode.baseRotation;
      const glm::quat blendRotation =
          blendNode.hasRotation ? blendNode.rotation : assetNode.baseRotation;
      mixedNode.rotation =
          glm::normalize(glm::slerp(baseRotation, blendRotation, weight));
    }

    mixedNode.hasScale = baseNode.hasScale || blendNode.hasScale;
    if (mixedNode.hasScale) {
      const glm::vec3 baseScale =
          baseNode.hasScale ? baseNode.scale : assetNode.baseScale;
      const glm::vec3 blendScale =
          blendNode.hasScale ? blendNode.scale : assetNode.baseScale;
      mixedNode.scale = glm::mix(baseScale, blendScale, weight);
    }
  }

  return mixedPose;
}

void MeshVisualizer::render(const glm::mat4 &worldTransform,
                            const DL::FrameContext &ctx) {
  if (renderDevice_ == nullptr || !pipeline_.valid() || asset_ == nullptr) {
    return;
  }

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, extractPosition(worldTransform));
  model = model * glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform));

  const AnimationPose pose = currentAnimationPose();

  std::unordered_map<int, std::vector<glm::mat4>> skinMatricesBySkin;
  std::vector<glm::mat4> animatedNodeWorldTransforms;
  if (!pose.nodes.empty()) {
    animatedNodeWorldTransforms = evaluateNodeWorldTransforms(*asset_, pose);
    for (const auto &submesh : submeshes_) {
      if (submesh.skinIndex < 0 ||
          skinMatricesBySkin.contains(submesh.skinIndex)) {
        continue;
      }
      skinMatricesBySkin.emplace(
          submesh.skinIndex, computeSkinMatrices(*asset_, submesh.skinIndex, pose));
    }
  }

  for (const auto &submesh : submeshes_) {
    if (!submesh.mesh.valid() || !submesh.texture.valid()) {
      continue;
    }

    DL::DrawCommand command;
    glm::mat4 submeshModel = model;
    if (submesh.skinIndex < 0 && submesh.sourceNodeIndex >= 0 &&
        static_cast<std::size_t>(submesh.sourceNodeIndex) <
            animatedNodeWorldTransforms.size() &&
        static_cast<std::size_t>(submesh.sourceNodeIndex) < asset_->nodes.size()) {
      const auto &node =
          asset_->nodes[static_cast<std::size_t>(submesh.sourceNodeIndex)];
      const glm::mat4 nodeAnimationDelta =
          animatedNodeWorldTransforms[static_cast<std::size_t>(
              submesh.sourceNodeIndex)] *
          glm::inverse(node.worldTransform);
      submeshModel = model * nodeAnimationDelta;
    }

    command.mesh = submesh.mesh;
    command.pipeline = pipeline_;
    command.texture = submesh.texture;
    command.uniforms.push_back(
        DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
    command.uniforms.push_back(DL::UniformValue::makeMat4("model", submeshModel));
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
    const auto skinMatrixIt = skinMatricesBySkin.find(submesh.skinIndex);
    const bool useSkinning =
        skinMatrixIt != skinMatricesBySkin.end() && !skinMatrixIt->second.empty();
    command.uniforms.push_back(
        DL::UniformValue::makeInt("useSkinning", useSkinning ? 1 : 0));
    if (useSkinning) {
      command.uniforms.push_back(
          DL::UniformValue::makeMat4Array("boneMatrices", skinMatrixIt->second));
    }
    command.uniforms.push_back(
        DL::UniformValue::makeInt("debugNormals", debugNormals_ ? 1 : 0));
    renderDevice_->draw(command);
  }
}

TextureHandle MeshVisualizer::createFallbackTexture(bool &sharedTexture) {
  if (resourceCache_ != nullptr) {
    sharedTexture = true;
    return resourceCache_->acquireWhiteTexture();
  }
  const std::uint8_t pixel[] = {255, 255, 255, 255};
  return renderDevice_->createTexture({.pixels = pixel,
                                       .width = 1,
                                       .height = 1,
                                       .format = TextureFormat::RGBA8,
                                       .generateMipmaps = false});
}

TextureHandle createTextureFromPixels(IRenderDevice &renderDevice,
                                      const std::vector<std::uint8_t> &pixels,
                                      std::uint32_t width,
                                      std::uint32_t height) {
  if (pixels.empty()) {
    return {};
  }
  return renderDevice.createTexture({.pixels = pixels.data(),
                                     .width = width,
                                     .height = height,
                                     .format = TextureFormat::RGBA8,
                                     .generateMipmaps = true});
}

TextureHandle loadTextureFile(IRenderDevice &renderDevice, std::string_view path) {
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_set_flip_vertically_on_load(false);
  unsigned char *pixels =
      stbi_load(std::string(path).c_str(), &width, &height, &channels, 4);
  if (pixels == nullptr || width <= 0 || height <= 0) {
    stbi_image_free(pixels);
    return {};
  }

  const auto texture = renderDevice.createTexture(
      {.pixels = pixels,
       .width = static_cast<std::uint32_t>(width),
       .height = static_cast<std::uint32_t>(height),
       .format = TextureFormat::RGBA8,
       .generateMipmaps = true});
  stbi_image_free(pixels);
  return texture;
}

TextureHandle MeshVisualizer::loadTexture(const MeshAssetSubmesh &submesh,
                                          bool &sharedTexture) {
  if (!submesh.texturePath.empty() && codeBook_ != nullptr &&
      submesh.texturePath.ends_with(".basis")) {
    auto texture = resourceCache_ != nullptr
                       ? resourceCache_->acquireBasisTexture(submesh.texturePath,
                                                             *codeBook_)
                       : renderDevice_->createBasisTexture(submesh.texturePath,
                                                           *codeBook_);
    if (texture.valid()) {
      sharedTexture = resourceCache_ != nullptr;
      return texture;
    }
  }
  if (!submesh.encodedTextureData.empty()) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char *pixels = stbi_load_from_memory(
        submesh.encodedTextureData.data(),
        static_cast<int>(submesh.encodedTextureData.size()), &width, &height,
        &channels, 4);
    if (pixels != nullptr && width > 0 && height > 0) {
      const auto texture = renderDevice_->createTexture(
          {.pixels = pixels,
           .width = static_cast<std::uint32_t>(width),
           .height = static_cast<std::uint32_t>(height),
           .format = TextureFormat::RGBA8,
           .generateMipmaps = true});
      stbi_image_free(pixels);
      if (texture.valid()) {
        return texture;
      }
    } else {
      stbi_image_free(pixels);
    }
  }
  if (!submesh.texturePath.empty()) {
    auto texture = loadTextureFile(*renderDevice_, submesh.texturePath);
    if (texture.valid()) {
      return texture;
    }
  }
  return createFallbackTexture(sharedTexture);
}

} // namespace DL

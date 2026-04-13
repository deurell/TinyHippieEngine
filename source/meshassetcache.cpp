#include "meshassetcache.h"

namespace DL {

std::shared_ptr<const MeshAsset> MeshAssetCache::load(std::string_view path) {
  const std::string key(path);
  const auto it = meshAssets_.find(key);
  if (it != meshAssets_.end()) {
    return it->second;
  }

  auto asset = std::make_shared<MeshAsset>(loadMeshAsset(path));
  auto inserted = meshAssets_.emplace(key, asset);
  return inserted.first->second;
}

} // namespace DL

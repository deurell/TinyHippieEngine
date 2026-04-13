#pragma once

#include "meshasset.h"
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace DL {

class MeshAssetCache {
public:
  std::shared_ptr<const MeshAsset> load(std::string_view path);

private:
  std::unordered_map<std::string, std::shared_ptr<const MeshAsset>> meshAssets_;
};

} // namespace DL

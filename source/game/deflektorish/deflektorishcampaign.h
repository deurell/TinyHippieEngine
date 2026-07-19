#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace Deflektorish {

class Campaign {
public:
  void loadDefaultLevelPaths(const std::string &levelRoot, int levelCount) {
    levelPaths_.clear();
    for (int i = 1; i <= levelCount; ++i) {
      std::string index = std::to_string(i);
      if (i < 10) {
        index.insert(index.begin(), '0');
      }
      levelPaths_.push_back(levelRoot + "level_" + index + ".json");
    }
    currentLevelIndex_ = 0;
    score_ = 0;
  }

  [[nodiscard]] const std::vector<std::string> &levelPaths() const {
    return levelPaths_;
  }

  [[nodiscard]] bool hasLevels() const { return !levelPaths_.empty(); }
  [[nodiscard]] std::size_t currentLevelIndex() const {
    return currentLevelIndex_;
  }
  [[nodiscard]] int score() const { return score_; }

  void setCurrentLevelIndex(std::size_t levelIndex, std::size_t levelCount) {
    currentLevelIndex_ = levelCount > 0 ? levelIndex % levelCount : 0;
  }

  void resetScore() { score_ = 0; }
  void addScore(int amount) { score_ = std::max(score_ + amount, 0); }
  void setScore(int score) { score_ = std::max(score, 0); }

private:
  std::vector<std::string> levelPaths_;
  std::size_t currentLevelIndex_ = 0;
  int score_ = 0;
};

} // namespace Deflektorish

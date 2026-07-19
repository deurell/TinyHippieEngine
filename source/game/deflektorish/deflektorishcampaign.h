#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Deflektorish {

struct HighScoreEntry {
  std::string initials = "AAA";
  int score = 0;
};

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
  [[nodiscard]] const std::vector<HighScoreEntry> &highScores() const {
    return highScores_;
  }
  [[nodiscard]] bool qualifiesHighScore(int score) const {
    if (highScores_.size() < maxHighScores_) {
      return true;
    }
    return std::max(score, 0) > highScores_.back().score;
  }

  void setCurrentLevelIndex(std::size_t levelIndex, std::size_t levelCount) {
    currentLevelIndex_ = levelCount > 0 ? levelIndex % levelCount : 0;
  }

  void resetScore() { score_ = 0; }
  void addScore(int amount) { score_ = std::max(score_ + amount, 0); }
  void setScore(int score) { score_ = std::max(score, 0); }
  std::size_t recordHighScore(std::string initials, int score) {
    if (initials.empty()) {
      initials = "AAA";
    }
    initials.resize(3, ' ');
    const std::string normalizedInitials = initials;
    const int normalizedScore = std::max(score, 0);
    HighScoreEntry entry{std::move(initials), normalizedScore};
    highScores_.push_back(std::move(entry));
    std::sort(highScores_.begin(), highScores_.end(),
              [](const HighScoreEntry &a, const HighScoreEntry &b) {
                return a.score > b.score;
              });
    if (highScores_.size() > maxHighScores_) {
      highScores_.resize(maxHighScores_);
    }
    for (std::size_t i = 0; i < highScores_.size(); ++i) {
      if (highScores_[i].initials == normalizedInitials &&
          highScores_[i].score == normalizedScore) {
        return i;
      }
    }
    return highScores_.size();
  }

private:
  static constexpr std::size_t maxHighScores_ = 5;
  std::vector<std::string> levelPaths_;
  std::vector<HighScoreEntry> highScores_{
      {"ACE", 98500}, {"LUX", 84200}, {"RAY", 73150},
      {"KID", 60900}, {"CPU", 1},
  };
  std::size_t currentLevelIndex_ = 0;
  int score_ = 0;
};

} // namespace Deflektorish

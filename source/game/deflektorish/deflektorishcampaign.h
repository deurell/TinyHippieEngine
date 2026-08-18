#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <charconv>
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
  [[nodiscard]] bool hasNextLevel(std::size_t levelCount) const {
    return currentLevelIndex_ + 1 < levelCount;
  }
  [[nodiscard]] const std::vector<HighScoreEntry> &highScores() const {
    return highScores_;
  }
  [[nodiscard]] bool qualifiesHighScore(int score) const {
    if (highScores_.size() < maxHighScores_) {
      return true;
    }
    return std::max(score, 0) > highScores_.back().score;
  }
  [[nodiscard]] std::string serializeHighScores() const {
    std::string data = "{\n  \"version\": 1,\n  \"highScores\": [\n";
    for (const HighScoreEntry &entry : highScores_) {
      std::string initials = entry.initials;
      initials.resize(3, ' ');
      data += "    { \"initials\": \"";
      data += escapeJson(initials);
      data += "\", \"score\": ";
      data += std::to_string(std::max(entry.score, 0));
      data += " }";
      if (&entry != &highScores_.back()) {
        data += ',';
      }
      data += '\n';
    }
    data += "  ]\n}\n";
    return data;
  }

  void setCurrentLevelIndex(std::size_t levelIndex, std::size_t levelCount) {
    currentLevelIndex_ =
        levelCount > 0 ? std::min(levelIndex, levelCount - 1) : 0;
  }

  void resetScore() { score_ = 0; }
  void addScore(int amount) { score_ = std::max(score_ + amount, 0); }
  void setScore(int score) { score_ = std::max(score, 0); }
  bool loadHighScoresFromText(const std::string &data) {
    std::vector<HighScoreEntry> loaded;
    std::size_t cursor = data.find("\"highScores\"");
    if (cursor == std::string::npos) {
      return false;
    }
    cursor = data.find('[', cursor);
    const std::size_t endArray = data.find(']', cursor);
    if (cursor == std::string::npos || endArray == std::string::npos) {
      return false;
    }
    while (cursor < endArray) {
      const std::size_t initialsKey = data.find("\"initials\"", cursor);
      if (initialsKey == std::string::npos || initialsKey >= endArray) {
        break;
      }
      const std::size_t initialsColon = data.find(':', initialsKey);
      const std::size_t initialsQuote = data.find('"', initialsColon);
      const std::size_t initialsEnd = data.find('"', initialsQuote + 1);
      const std::size_t scoreKey = data.find("\"score\"", initialsEnd);
      if (initialsColon == std::string::npos ||
          initialsQuote == std::string::npos ||
          initialsEnd == std::string::npos || scoreKey == std::string::npos ||
          scoreKey >= endArray) {
        break;
      }
      const std::size_t scoreColon = data.find(':', scoreKey);
      if (scoreColon == std::string::npos) {
        break;
      }
      std::size_t scoreBegin = scoreColon + 1;
      while (scoreBegin < data.size() &&
             std::isspace(static_cast<unsigned char>(data[scoreBegin]))) {
        ++scoreBegin;
      }
      std::size_t scoreEnd = scoreBegin;
      while (scoreEnd < data.size() &&
             (std::isdigit(static_cast<unsigned char>(data[scoreEnd])) ||
              data[scoreEnd] == '-')) {
        ++scoreEnd;
      }
      int score = 0;
      const auto result =
          std::from_chars(data.data() + scoreBegin, data.data() + scoreEnd,
                          score);
      if (result.ec == std::errc()) {
        std::string initials =
            data.substr(initialsQuote + 1, initialsEnd - initialsQuote - 1);
        loaded.push_back({std::move(initials), std::max(score, 0)});
      }
      cursor = scoreEnd;
    }
    if (loaded.empty()) {
      return false;
    }
    setHighScores(std::move(loaded));
    return true;
  }
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
  static std::string escapeJson(const std::string &value) {
    std::string escaped;
    for (char c : value) {
      if (c == '"' || c == '\\') {
        escaped.push_back('\\');
      }
      escaped.push_back(c);
    }
    return escaped;
  }

  void setHighScores(std::vector<HighScoreEntry> scores) {
    const std::vector<HighScoreEntry> defaults = defaultHighScores();
    for (const HighScoreEntry &entry : defaults) {
      const bool alreadyPresent =
          std::any_of(scores.begin(), scores.end(),
                      [&](const HighScoreEntry &score) {
                        return score.initials == entry.initials &&
                               score.score == entry.score;
                      });
      if (!alreadyPresent) {
        scores.push_back(entry);
      }
    }
    for (HighScoreEntry &entry : scores) {
      if (entry.initials.empty()) {
        entry.initials = "AAA";
      }
      entry.initials.resize(3, ' ');
      entry.score = std::max(entry.score, 0);
    }
    std::sort(scores.begin(), scores.end(),
              [](const HighScoreEntry &a, const HighScoreEntry &b) {
                return a.score > b.score;
              });
    if (scores.size() > maxHighScores_) {
      scores.resize(maxHighScores_);
    }
    highScores_ = std::move(scores);
  }

  static std::vector<HighScoreEntry> defaultHighScores() {
    return {
        {"ACE", 98500}, {"LUX", 84200}, {"RAY", 73150}, {"KID", 60900},
        {"MIR", 55400}, {"ZAP", 50850}, {"ORB", 46300}, {"ION", 41750},
        {"PIX", 38200}, {"VEX", 34600}, {"NIX", 31150}, {"QRT", 28700},
        {"JAM", 25350}, {"BPM", 22100}, {"CRT", 19650}, {"GLW", 17000},
        {"HUM", 13250}, {"TIN", 9400},  {"BYT", 5200}, {"CPU", 1},
    };
  }

  static constexpr std::size_t maxHighScores_ = 20;
  std::vector<std::string> levelPaths_;
  std::vector<HighScoreEntry> highScores_ = defaultHighScores();
  std::size_t currentLevelIndex_ = 0;
  int score_ = 0;
};

} // namespace Deflektorish

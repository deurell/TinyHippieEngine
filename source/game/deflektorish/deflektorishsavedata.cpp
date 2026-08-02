#include "game/deflektorish/deflektorishsavedata.h"

#include "game/deflektorish/deflektorishcampaign.h"
#include "logger.h"
#include "persistentstorage.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace Deflektorish {
namespace {

constexpr char kHighScoreStorageKey[] =
    "tiny_hippie.deflektorish.high_scores.v1";
constexpr char kLevelTimeStorageKey[] = "deflektorish_level_times.json";

std::string escapeJson(std::string_view value) {
  std::string escaped;
  for (const char c : value) {
    switch (c) {
    case '"':
    case '\\':
      escaped.push_back('\\');
      escaped.push_back(c);
      break;
    case '\n':
      escaped += "\\n";
      break;
    case '\r':
      escaped += "\\r";
      break;
    case '\t':
      escaped += "\\t";
      break;
    default:
      escaped.push_back(c);
      break;
    }
  }
  return escaped;
}

std::string appendLevelTime(std::optional<std::string> data,
                            std::string_view entry) {
  if (!data.has_value()) {
    return "{\n  \"version\": 1,\n  \"levelTimes\": [\n    " +
           std::string(entry) + "\n  ]\n}\n";
  }

  const std::size_t key = data->find("\"levelTimes\"");
  const std::size_t arrayBegin = data->find('[', key);
  const std::size_t arrayEnd = data->find(']', arrayBegin);
  if (key == std::string::npos || arrayBegin == std::string::npos ||
      arrayEnd == std::string::npos) {
    return "{\n  \"version\": 1,\n  \"levelTimes\": [\n    " +
           std::string(entry) + "\n  ]\n}\n";
  }

  const bool hasEntries = std::any_of(
      data->begin() + static_cast<std::ptrdiff_t>(arrayBegin + 1),
      data->begin() + static_cast<std::ptrdiff_t>(arrayEnd),
      [](char c) { return c == '{'; });
  data->insert(arrayEnd, (hasEntries ? ",\n    " : "\n    ") +
                             std::string(entry) + "\n  ");
  return *data;
}

} // namespace

void loadHighScores(Campaign &campaign) {
  const std::optional<std::string> data =
      DL::PersistentStorage::readText(kHighScoreStorageKey);
  if (!data.has_value()) {
    return;
  }
  if (!campaign.loadHighScoresFromText(*data)) {
    DL::Logger::instance().logEvent(
        DL::LogLevel::Warning, "storage",
        "deflektor_high_scores_invalid");
  }
}

void saveHighScores(const Campaign &campaign) {
  if (!DL::PersistentStorage::writeText(kHighScoreStorageKey,
                                        campaign.serializeHighScores())) {
    DL::Logger::instance().logEvent(
        DL::LogLevel::Warning, "storage",
        "deflektor_high_scores_save_failed");
  }
}

void recordLevelTime(std::size_t levelIndex, std::string_view levelName,
                     float seconds, bool debugSkip) {
  std::ostringstream entry;
  entry << "{ \"level\": " << levelIndex + 1 << ", \"name\": \""
        << escapeJson(levelName) << "\", \"seconds\": " << std::fixed
        << std::setprecision(3) << std::max(seconds, 0.0f)
        << ", \"recordedAtUnix\": " << std::time(nullptr)
        << ", \"debugSkip\": " << (debugSkip ? "true" : "false") << " }";

  const std::string json = appendLevelTime(
      DL::PersistentStorage::readText(kLevelTimeStorageKey), entry.str());
  if (!DL::PersistentStorage::writeText(kLevelTimeStorageKey, json)) {
    DL::Logger::instance().logEvent(DL::LogLevel::Warning, "storage",
                                    "deflektor_level_time_save_failed");
    return;
  }
  DL::Logger::instance().logEvent(
      DL::LogLevel::Info, "playtest", "deflektor_level_time_recorded",
      entry.str());
}

} // namespace Deflektorish

#include "game/deflektorish/deflektorishsavedata.h"

#include "game/deflektorish/deflektorishcampaign.h"
#include "logger.h"
#include "persistentstorage.h"

#include <optional>
#include <string>

namespace Deflektorish {
namespace {

constexpr char kHighScoreStorageKey[] =
    "tiny_hippie.deflektorish.high_scores.v1";

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

} // namespace Deflektorish

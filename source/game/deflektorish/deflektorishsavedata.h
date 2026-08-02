#pragma once

#include <cstddef>
#include <string_view>

namespace Deflektorish {

class Campaign;

void loadHighScores(Campaign &campaign);
void saveHighScores(const Campaign &campaign);
void recordLevelTime(std::size_t levelIndex, std::string_view levelName,
                     float seconds, bool debugSkip);

} // namespace Deflektorish

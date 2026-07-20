#include "game/deflektorish/deflektorishcampaign.h"
#include <gtest/gtest.h>

TEST(DeflektorishCampaignTest, BuildsDefaultLevelPaths) {
  Deflektorish::Campaign campaign;

  campaign.loadDefaultLevelPaths("Levels/", 3);

  ASSERT_EQ(campaign.levelPaths().size(), 3u);
  EXPECT_EQ(campaign.levelPaths()[0], "Levels/level_01.json");
  EXPECT_EQ(campaign.levelPaths()[2], "Levels/level_03.json");
  EXPECT_TRUE(campaign.hasLevels());
}

TEST(DeflektorishCampaignTest, WrapsCurrentLevelIndex) {
  Deflektorish::Campaign campaign;
  campaign.loadDefaultLevelPaths("Levels/", 3);

  campaign.setCurrentLevelIndex(4, campaign.levelPaths().size());

  EXPECT_EQ(campaign.currentLevelIndex(), 1u);
}

TEST(DeflektorishCampaignTest, TracksNonNegativeScore) {
  Deflektorish::Campaign campaign;

  campaign.addScore(120);
  campaign.addScore(-200);
  EXPECT_EQ(campaign.score(), 0);

  campaign.setScore(50);
  EXPECT_EQ(campaign.score(), 50);
}

TEST(DeflektorishCampaignTest, RecordsHighScoresInDescendingOrder) {
  Deflektorish::Campaign campaign;

  const std::size_t rank = campaign.recordHighScore("YOU", 90000);

  ASSERT_EQ(campaign.highScores().size(), 5u);
  EXPECT_EQ(rank, 1u);
  EXPECT_EQ(campaign.highScores()[0].initials, "ACE");
  EXPECT_EQ(campaign.highScores()[1].initials, "YOU");
  EXPECT_EQ(campaign.highScores()[1].score, 90000);
}

TEST(DeflektorishCampaignTest, HighScoreInitialsStayThreeCharacters) {
  Deflektorish::Campaign campaign;

  campaign.recordHighScore("A", 100000);

  ASSERT_FALSE(campaign.highScores().empty());
  EXPECT_EQ(campaign.highScores()[0].initials, "A  ");
}

TEST(DeflektorishCampaignTest, ReportsWhetherScoreQualifiesForTable) {
  Deflektorish::Campaign campaign;

  EXPECT_FALSE(campaign.qualifiesHighScore(1));
  EXPECT_TRUE(campaign.qualifiesHighScore(100));
  EXPECT_TRUE(campaign.qualifiesHighScore(50000));
}

TEST(DeflektorishCampaignTest, SerializesHighScoresAsJson) {
  Deflektorish::Campaign campaign;
  campaign.recordHighScore("YOU", 90000);

  const std::string data = campaign.serializeHighScores();

  EXPECT_NE(data.find("\"highScores\""), std::string::npos);
  EXPECT_NE(data.find("\"initials\": \"YOU\""), std::string::npos);
  EXPECT_NE(data.find("\"score\": 90000"), std::string::npos);
}

TEST(DeflektorishCampaignTest, LoadsHighScoresFromJson) {
  Deflektorish::Campaign campaign;
  const bool loaded = campaign.loadHighScoresFromText(
      "{\n"
      "  \"version\": 1,\n"
      "  \"highScores\": [\n"
      "    { \"initials\": \"ZZZ\", \"score\": 12 },\n"
      "    { \"initials\": \"AAA\", \"score\": 1200 }\n"
      "  ]\n"
      "}\n");

  ASSERT_TRUE(loaded);
  ASSERT_EQ(campaign.highScores().size(), 2u);
  EXPECT_EQ(campaign.highScores()[0].initials, "AAA");
  EXPECT_EQ(campaign.highScores()[0].score, 1200);
  EXPECT_EQ(campaign.highScores()[1].initials, "ZZZ");
  EXPECT_EQ(campaign.highScores()[1].score, 12);
}

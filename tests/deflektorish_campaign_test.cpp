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

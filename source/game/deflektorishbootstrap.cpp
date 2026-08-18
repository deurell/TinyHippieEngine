#include "game/deflektorishbootstrap.h"

#include "app.h"
#include "game/deflektorish/deflektorishcampaign.h"
#include "game/deflektorish/deflektorishsavedata.h"
#include "game/deflektorish/deflektorishsoundmap.h"
#include "game/scenes/deflektorishintroscene.h"
#include "game/scenes/deflektorishscene.h"
#include <algorithm>
#include <map>
#include <optional>
#include <random>
#include <vector>

namespace {

constexpr float kPostBumpDuration = 1.35f;
constexpr float kPostBumpStrengthScale = 0.026f;
constexpr float kPostBumpMaxStrength = 0.052f;

float randomRange(float minValue, float maxValue) {
  static std::mt19937 rng{std::random_device{}()};
  std::uniform_real_distribution<float> distribution(minValue, maxValue);
  return distribution(rng);
}

class DeflektorishBootstrap final : public DL::AppBootstrap {
public:
  void configure(DL::App &app) override {
    campaign_.loadDefaultLevelPaths("Resources/Game/Deflektorish/Levels/", 20);
    Deflektorish::loadHighScores(campaign_);
    app.addPostProcessEffect(
        "Deflektor Bump", "Shaders/deflektorish_post.frag",
        {DL::UniformValue::makeVec4("bump1", {0.0f, 0.0f, 1.0f, 0.0f}),
         DL::UniformValue::makeVec4("bump2", {0.0f, 0.0f, 1.0f, 0.0f})});

    app.registerScene([this, &app] {
      return std::make_unique<DeflektorishIntroScene>(
          app.renderDevice(), app.renderResourceCache(),
          &campaign_.highScores(), pendingInitialsScore_,
          [this](int score, std::string initials) {
            campaign_.recordHighScore(std::move(initials), score);
            Deflektorish::saveHighScores(campaign_);
            pendingInitialsScore_.reset();
          },
          [this, &app](Deflektorish::Sound sound, glm::vec2, float energy) {
            submitSound(app, sound, energy);
          },
          [&app] { app.requestNextScene(); });
    });
    app.registerScene([this, &app] {
      return std::make_unique<DeflektorishScene>(
          app.renderDevice(), app.renderResourceCache(),
          [this](glm::vec2 uv, float strength) { submitPostBump(uv, strength); },
          [this, &app](Deflektorish::Sound sound, glm::vec2, float energy) {
            submitSound(app, sound, energy);
          },
          [this, &app](int score) {
            pendingInitialsScore_ =
                campaign_.qualifiesHighScore(score) ? std::optional<int>(score)
                                                    : std::nullopt;
            app.requestPreviousScene();
          });
    });
  }

  void resourcesReady(DL::App &app) override {
    soundMap_ = Deflektorish::loadSoundMap(
        "Resources/Game/Deflektorish/Audio/sounds.json");
    for (const auto &[sound, event] : soundMap_.events) {
      app.audioSystem().loadClip(Deflektorish::soundEventKey(sound),
                                 soundMap_.audioRoot + "/" + event.clip,
                                 static_cast<std::size_t>(event.poolSize));
    }
  }

  void update(DL::App &, float deltaTime) override {
    for (auto &bump : postBumps_) {
      bump.age += deltaTime;
    }
    postBumps_.erase(
        std::remove_if(postBumps_.begin(), postBumps_.end(),
                       [](const PostBump &bump) {
                         return bump.age >= kPostBumpDuration;
                       }),
        postBumps_.end());
  }

  void beforeRender(DL::App &app) override {
    for (std::size_t index = 0; index < 2; ++index) {
      glm::vec4 value{0.0f, 0.0f, 1.0f, 0.0f};
      if (index < postBumps_.size()) {
        const auto &bump = postBumps_[index];
        value = {bump.uv.x, bump.uv.y, bump.age / kPostBumpDuration,
                 bump.strength};
      }
      app.setPostProcessVec4("Deflektor Bump",
                             index == 0 ? "bump1" : "bump2", value);
    }
  }

private:
  struct PostBump {
    glm::vec2 uv{0.0f};
    float age = 0.0f;
    float strength = 0.0f;
  };

  void submitPostBump(glm::vec2 uv, float strength) {
    postBumps_.insert(postBumps_.begin(),
                      {glm::clamp(uv, glm::vec2(0.0f), glm::vec2(1.0f)), 0.0f,
                       std::min(std::max(strength, 0.0f) *
                                    kPostBumpStrengthScale,
                                kPostBumpMaxStrength)});
    if (postBumps_.size() > 2) {
      postBumps_.resize(2);
    }
  }

  void submitSound(DL::App &app, Deflektorish::Sound sound, float energy) {
    const auto *event = soundMap_.find(sound);
    if (event == nullptr) {
      return;
    }
    auto &active = activeSounds_[sound];
    active.erase(std::remove_if(active.begin(), active.end(),
                                [&app](DL::AudioSystem::SoundId id) {
                                  return !app.audioSystem().isPlaying(id);
                                }),
                 active.end());
    if (active.size() >=
        static_cast<std::size_t>(std::max(event->maxConcurrent, 1))) {
      return;
    }
    const float volume =
        std::clamp(event->volume + energy * event->volumePerEnergy, 0.0f,
                   event->maxVolume);
    const auto id = app.audioSystem().playOneShot(
        Deflektorish::soundEventKey(sound), DL::AudioGroup::SFX, volume,
        randomRange(event->pitchMin, event->pitchMax));
    if (id != DL::AudioSystem::kInvalidSoundId) {
      active.push_back(id);
    }
  }

  Deflektorish::Campaign campaign_;
  Deflektorish::SoundMapConfig soundMap_;
  std::map<Deflektorish::Sound, std::vector<DL::AudioSystem::SoundId>>
      activeSounds_;
  std::vector<PostBump> postBumps_;
  std::optional<int> pendingInitialsScore_;
};

} // namespace

std::unique_ptr<DL::AppBootstrap> createDeflektorishBootstrap() {
  return std::make_unique<DeflektorishBootstrap>();
}

#ifndef __RAINBOW_COLOR_FADE_COMMAND_HPP__
#define __RAINBOW_COLOR_FADE_COMMAND_HPP__

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "core/effectCommand.hpp"

namespace core {

class RainbowColorFadeCommand : public EffectCommand {
    using Color = std::array<uint8_t, 3>;

   public:
    explicit RainbowColorFadeCommand(std::chrono::steady_clock::duration duration)
        : fade_duration(duration) {
        rainbow_colors = {
            Color{MAX_CHANNEL_VALUE, MIN_CHANNEL_VALUE, MIN_CHANNEL_VALUE},
            Color{MAX_CHANNEL_VALUE, MAX_CHANNEL_VALUE, MIN_CHANNEL_VALUE},
            Color{MIN_CHANNEL_VALUE, MAX_CHANNEL_VALUE, MIN_CHANNEL_VALUE},
            Color{MIN_CHANNEL_VALUE, MAX_CHANNEL_VALUE, MAX_CHANNEL_VALUE},
            Color{MIN_CHANNEL_VALUE, MIN_CHANNEL_VALUE, MAX_CHANNEL_VALUE},
            Color{MAX_CHANNEL_VALUE, MIN_CHANNEL_VALUE, MAX_CHANNEL_VALUE},
        };

        buildSequense();
    }

    Color execute(std::chrono::steady_clock::duration interval) override;

    bool isFinished() const override { return false; }

    std::unique_ptr<EffectCommand> clone() override;
    std::unique_ptr<EffectCommand> makeNew(
        EffectParams const& params) override;

   private:
    void buildSequense();

    std::chrono::steady_clock::duration fade_duration;
    std::vector<std::unique_ptr<EffectCommand>> sequence;
    std::vector<std::unique_ptr<EffectCommand>> prototype;
    std::vector<Color> rainbow_colors;
    std::size_t current_command_idx{0};
};

}  // namespace core

#endif  // !__RAINBOW_COLOR_FADE_COMMAND_HPP__

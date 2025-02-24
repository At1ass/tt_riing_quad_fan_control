#ifndef __FADE_COMMAND_HPP__
#define __FADE_COMMAND_HPP__

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>

#include "core/effectCommand.hpp"

namespace core {
class FadeCommand : public EffectCommand {
    using Color = std::array<uint8_t, 3>;
    using Duration = std::chrono::steady_clock::duration;

   public:
    FadeCommand(Color const& start, Color const& end, Duration dur)
        : start_color(start),
          end_color(end),
          current_color(start_color),
          duration(dur),
          elapsed(Duration::zero()),
          start_time(std::chrono::steady_clock::now()) {}

    Color execute(Duration interval) override;

    bool isFinished() const override { return finished; }

    std::unique_ptr<EffectCommand> clone() override;
    std::unique_ptr<EffectCommand> makeNew(EffectParams const& params) override;

   private:
    Color start_color;
    Color end_color;
    Color current_color;
    Duration duration;
    Duration elapsed;
    std::chrono::steady_clock::time_point start_time;
    bool finished{false};
};

}  // namespace core

#endif  // !__FADE_COMMAND_HPP__

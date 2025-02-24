#ifndef __STATIC_COLOR_COMMAND_HPP__
#define __STATIC_COLOR_COMMAND_HPP__

#include <chrono>
#include <cstdint>
#include <memory>

#include "core/effectCommand.hpp"

namespace core {

class StaticColorCommand : public EffectCommand {
    using Color = std::array<uint8_t, 3>;

   public:
    StaticColorCommand(uint8_t r, uint8_t g, uint8_t b,
                       std::chrono::steady_clock::duration duration)
        : r(r),
          g(g),
          b(b),
          duration(duration),
          elapsed(std::chrono::steady_clock::duration::zero()) {}

    Color execute(std::chrono::steady_clock::duration interval) override;

    bool isFinished() const override { return elapsed >= duration; }

    std::unique_ptr<EffectCommand> clone() override;
    std::unique_ptr<EffectCommand> makeNew(EffectParams const& params) override;

    static std::unique_ptr<EffectCommand> makeStaticColorCommand(
        uint8_t r, uint8_t g, uint8_t b,
        std::chrono::steady_clock::duration duration);

   private:
    uint8_t r, g, b;
    std::chrono::steady_clock::duration duration;
    std::chrono::steady_clock::duration elapsed;
};

}  // namespace core

#endif  // !__STATIC_COLOR_COMMAND_HPP__
